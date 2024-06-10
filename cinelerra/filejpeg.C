// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "edit.h"
#include "file.h"
#include "filejpeg.h"
#include "interlacemodes.h"
#include "language.h"
#include "mainerror.h"
#include "mwindow.h"
#include "paramlist.h"
#include "paramlistwindow.h"
#include "tmpframecache.h"
#include "vframe.h"

#include <setjmp.h>
#include <string.h>

#define FILEJPEG_VCODEC_IX 0
#define JPEG_ENC_CONFIG_NAME "jpeg:enc"

#define PARAM_QUALITY "quality"

struct paramlist_defaults FileJPEG::encoder_params[] =
{
	{ PARAM_QUALITY, N_("Quality:"), PARAMTYPE_INT, 100 },
	{ 0, 0, 0, 0 }
};

#if JPEG_LIB_VERSION < 80 && !defined(MEM_SRCDST_SUPPORTED)
struct my_src_mgr
{
	struct jpeg_source_mgr jpeg_src;
	unsigned char *src_data;
	size_t src_size;
};

struct my_dst_mgr
{
	struct jpeg_destination_mgr jpeg_dst;
	unsigned char *dst_data;
	size_t dst_size;
	size_t dst_filled;
};

static void init_jpeg_src(j_decompress_ptr cinfo)
{
	struct my_src_mgr *ms = (struct my_src_mgr *)cinfo->src;

	cinfo->src->next_input_byte = ms->src_data;
	cinfo->src->bytes_in_buffer = ms->src_size;
}

static boolean fill_jpeg_src(j_decompress_ptr cinfo)
{
	return TRUE;
}

static void skip_jpeg_src(j_decompress_ptr cinfo, long num_bytes)
{
}

static void term_jpeg_src(j_decompress_ptr cinfo)
{
}

static void init_jpeg_dst(j_compress_ptr cinfo)
{
	struct my_dst_mgr *ms = (struct my_dst_mgr *)cinfo->dest;

	cinfo->dest->next_output_byte = ms->dst_data;
	cinfo->dest->free_in_buffer = ms->dst_size;
	ms->dst_filled = 0;
}

static boolean empty_jpeg_dst(j_compress_ptr cinfo)
{
	return TRUE;
}

static void term_jpeg_dst(j_compress_ptr cinfo)
{
	struct my_dst_mgr *ms = (struct my_dst_mgr *)cinfo->dest;

	ms->dst_filled = ms->dst_size - cinfo->dest->free_in_buffer;
}
#endif

struct error_mgr
{
	struct jpeg_error_mgr jpeglib_err;
	jmp_buf setjmp_buffer;
};

void jpg_err_exit(j_common_ptr cinfo)
{
	struct error_mgr *mgp = (struct error_mgr *)cinfo->err;
	longjmp(mgp->setjmp_buffer, 1);
}

FileJPEG::FileJPEG(Asset *asset, File *file)
 : FileList(asset, file, "JPEGLIST", ".jpg", FILE_JPEG, FILE_JPEG_LIST)
{
	temp_frame = 0;
}

FileJPEG::~FileJPEG()
{
	delete temp_frame;
}


int FileJPEG::check_sig(Asset *asset)
{
	FILE *stream = fopen(asset->path, "rb");
	int l;

	if(stream)
	{
		char test[10];
		l = fread(test, 10, 1, stream);
		fclose(stream);

		if(l < 1) return 0;

		if(test[6] == 'J' && test[7] == 'F' && test[8] == 'I' && test[9] == 'F')
		{
			return 1;
		}
		else
		if(test[0] == 'J' && test[1] == 'P' && test[2] == 'E' && test[3] == 'G' && 
			test[4] == 'L' && test[5] == 'I' && test[6] == 'S' && test[7] == 'T')
		{
			return 1;
		}
	}

	if(strlen(asset->path) > 4)
	{
		int len = strlen(asset->path);
		if(!strncasecmp(asset->path + len - 4, ".jpg", 4))
			return 1;
	}
	return 0;
}

void FileJPEG::get_parameters(BC_WindowBase *parent_window, 
	Asset *asset, 
	BC_WindowBase* &format_window,
	int options)
{
	Paramlist *plist;

	if(options & SUPPORTS_VIDEO)
	{
		asset->encoder_parameters[FILEJPEG_VCODEC_IX] =
			Paramlist::construct("FileJPEG",
				asset->encoder_parameters[FILEJPEG_VCODEC_IX],
				encoder_params);

		plist = Paramlist::clone(asset->encoder_parameters[FILEJPEG_VCODEC_IX]);

		ParamlistThread thread(&plist,
			_("JPEG compression"), mwindow_global->get_window_icon(),
			&format_window);
		thread.run();

		if(!thread.win_result)
		{
			Param *parm;

			parm = plist->find(PARAM_QUALITY);

			if(parm->intvalue < 0)
				parm->intvalue = 0;
			if(parm->intvalue > 100)
				parm->intvalue = 100;

			if(!plist->equiv(asset->encoder_parameters[FILEJPEG_VCODEC_IX]))
				asset->encoder_parameters[FILEJPEG_VCODEC_IX]->copy_values(plist);
		}
		delete plist;
	}
}

void FileJPEG::show_jpeg_error(j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];

	(*cinfo->err->format_message)(cinfo, buffer);
	errormsg("libjpeg error: %s", buffer);
}

int FileJPEG::write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit)
{
	VFrame *work_frame;
	JSAMPROW row_pointer[1];
	struct jpeg_compress_struct cinfo;
	struct error_mgr jpeg_error;
	unsigned long clength;
	JPEGUnit *jpeg_unit = (JPEGUnit*)unit;
	int jpeg_quality = 100;

	if(frame->get_color_model() != BC_RGB888)
	{
		if(jpeg_unit->temp_frame && (jpeg_unit->temp_frame->get_w() != frame->get_w() ||
			jpeg_unit->temp_frame->get_h() != frame->get_h()))
		{
			delete jpeg_unit->temp_frame;
			jpeg_unit->temp_frame = 0;
		}
		if(!jpeg_unit->temp_frame)
			jpeg_unit->temp_frame = new VFrame(0, frame->get_w(),
				frame->get_h(), BC_RGB888);
		jpeg_unit->temp_frame->transfer_from(frame);
		work_frame = jpeg_unit->temp_frame;
	}
	else
		work_frame = frame;

	if(jpeg_unit->compressed)
		free(jpeg_unit->compressed);

	cinfo.err = jpeg_std_error(&jpeg_error.jpeglib_err);
	jpeg_error.jpeglib_err.error_exit = jpg_err_exit;

	if(setjmp(jpeg_error.setjmp_buffer))
	{
		show_jpeg_error((j_common_ptr)&cinfo);
		jpeg_destroy_compress(&cinfo);
		return 1;
	}

	jpeg_create_compress(&cinfo);

#if JPEG_LIB_VERSION >= 80 || defined(MEM_SRCDST_SUPPORTED)
	jpeg_unit->compressed = 0;
	clength = 0;
	jpeg_mem_dest(&cinfo, &jpeg_unit->compressed, &clength);
#else
	struct my_dst_mgr my_dst;
	size_t sz = work_frame->get_w() * work_frame->get_h() * 3;

	jpeg_unit->compressed = my_dst.dst_data = (unsigned char*)malloc(sz);
	my_dst.dst_size = sz;

	my_dst.jpeg_dst.init_destination = init_jpeg_dst;
	my_dst.jpeg_dst.empty_output_buffer = empty_jpeg_dst;
	my_dst.jpeg_dst.term_destination = term_jpeg_dst;

	cinfo.dest = (jpeg_destination_mgr *)&my_dst;
#endif

	cinfo.image_width = work_frame->get_w();
	cinfo.image_height = work_frame->get_h();
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	if(asset->encoder_parameters[FILEJPEG_VCODEC_IX])
		jpeg_quality = asset->encoder_parameters[FILEJPEG_VCODEC_IX]->get(PARAM_QUALITY, jpeg_quality);
	jpeg_set_quality(&cinfo, jpeg_quality, TRUE);
	jpeg_start_compress(&cinfo, TRUE);
	for(int i; i < cinfo.image_height; i++)
	{
		row_pointer[0] = work_frame->get_data() + i * work_frame->get_bytes_per_line();
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

#if JPEG_LIB_VERSION >= 80 || defined(MEM_SRCDST_SUPPORTED)
	data->set_compressed_memory(jpeg_unit->compressed, clength, clength);
#else
	data->set_compressed_memory(jpeg_unit->compressed, my_dst.dst_filled,
		my_dst.dst_size);
#endif
	return 0;
}

int FileJPEG::read_frame_header(const char *path)
{
	FILE *stream;
	struct jpeg_decompress_struct jpeg_decompress;
	struct error_mgr jpeg_error;

	if(!(stream = fopen(path, "rb")))
	{
		errormsg("Error while opening \"%s\" for reading. \n%m\n", asset->path);
		return 1;
	}

	jpeg_decompress.err = jpeg_std_error(&jpeg_error.jpeglib_err);
	jpeg_error.jpeglib_err.error_exit = jpg_err_exit;

	if(setjmp(jpeg_error.setjmp_buffer))
	{
		show_jpeg_error((j_common_ptr)&jpeg_decompress);
		jpeg_destroy_decompress(&jpeg_decompress);
		return 1;
	}

	jpeg_create_decompress(&jpeg_decompress);

	jpeg_stdio_src(&jpeg_decompress, stream);
	jpeg_read_header(&jpeg_decompress, TRUE);

	asset->interlace_mode = BC_ILACE_MODE_NOTINTERLACED;

	jpeg_destroy_decompress(&jpeg_decompress);
	fclose(stream);

	return 0;
}

int FileJPEG::read_frame(VFrame *output, VFrame *input)
{
	int cmodel;
	struct jpeg_decompress_struct jpeg_decompress;
	struct error_mgr jpeg_error;
	VFrame *work_frame;
	JSAMPROW row_pointer[1];

	jpeg_decompress.err = jpeg_std_error(&jpeg_error.jpeglib_err);
	jpeg_error.jpeglib_err.error_exit = jpg_err_exit;

	if(setjmp(jpeg_error.setjmp_buffer))
	{
		show_jpeg_error((j_common_ptr)&jpeg_decompress);
		jpeg_destroy_decompress(&jpeg_decompress);
		return 1;
	}
	jpeg_create_decompress(&jpeg_decompress);

#if JPEG_LIB_VERSION >= 80 || defined(MEM_SRCDST_SUPPORTED)
	jpeg_mem_src(&jpeg_decompress, input->get_data(),
		input->get_compressed_size());
#else
	struct my_src_mgr my_src;

	my_src.src_data = input->get_data();
	my_src.src_size = input->get_compressed_size();

	my_src.jpeg_src.init_source = init_jpeg_src;
	my_src.jpeg_src.fill_input_buffer = fill_jpeg_src;
	my_src.jpeg_src.skip_input_data = skip_jpeg_src;
	my_src.jpeg_src.resync_to_restart = jpeg_resync_to_restart;
	my_src.jpeg_src.term_source = term_jpeg_src;

	jpeg_decompress.src = (struct jpeg_source_mgr *)&my_src;
#endif

	jpeg_read_header(&jpeg_decompress, TRUE);
	jpeg_start_decompress(&jpeg_decompress);

	switch(jpeg_decompress.output_components)
	{
	case 3:
		cmodel = BC_RGB888;
		break;
	case 4:
		cmodel = BC_RGBA8888;
		break;
	}
	work_frame = BC_Resources::tmpframes.get_tmpframe(jpeg_decompress.image_width,
		jpeg_decompress.image_height, cmodel, "FileJPEG::read_frame");

	for(int i = 0; i < jpeg_decompress.output_height; i++)
	{
		row_pointer[0] = work_frame->get_row_ptr(i);
		jpeg_read_scanlines(&jpeg_decompress, row_pointer, 1);
	}

	output->transfer_from(work_frame);
	BC_Resources::tmpframes.release_frame(work_frame);

	jpeg_finish_decompress(&jpeg_decompress);
	jpeg_destroy_decompress(&jpeg_decompress);
	return 0;
}

void FileJPEG::save_render_options(Asset *asset)
{
	char pathbuf[BCTEXTLEN];

	asset->profile_config_path(JPEG_ENC_CONFIG_NAME, pathbuf);
	strcat(pathbuf, XML_CONFIG_EXT);

	Paramlist::save_paramlist(asset->encoder_parameters[FILEJPEG_VCODEC_IX],
		pathbuf, encoder_params);
}

void FileJPEG::load_render_options(Asset *asset)
{
	char pathbuf[BCTEXTLEN];

	asset->profile_config_path(JPEG_ENC_CONFIG_NAME, pathbuf);
	strcat(pathbuf, XML_CONFIG_EXT);

	delete asset->encoder_parameters[FILEJPEG_VCODEC_IX];
	asset->encoder_parameters[FILEJPEG_VCODEC_IX] =
		Paramlist::load_paramlist(pathbuf);
}

void FileJPEG::dump_jpeg_decompress_struct(jpeg_decompress_struct *jpeg, int indent)
{
	printf("%*sjpeg_decompress_struct %p dump:\n", indent, "", jpeg);
	indent +=2;
	printf("%*s[%d,%d], num_components %d '%s' -> '%s' precision %d\n", indent, "",
		jpeg->image_width, jpeg->image_height, jpeg->num_components,
		color_space_name(jpeg->jpeg_color_space),
		color_space_name(jpeg->out_color_space), jpeg->data_precision);
	printf("%*sout: [%d,%d], out_color_components %d output_components %d\n",
		indent, "", jpeg->output_width, jpeg->output_height,
		jpeg->out_color_components, jpeg->output_components);
}

const char *FileJPEG::color_space_name(J_COLOR_SPACE cspce)
{
	switch(cspce)
	{
	case JCS_GRAYSCALE:
		return "Monochrome";
	case JCS_RGB:
		return "mRGB";
	case JCS_YCbCr:
		return "YUV";
	case JCS_CMYK:
		return "CMYK";
	case JCS_YCCK:
		return "YCbCrK";
	case JCS_EXT_RGB:
		return "RGB";
	case JCS_EXT_BGR:
		return "BGR";
	case JCS_EXT_BGRX:
		return "BGRx";
	case JCS_EXT_XBGR:
		return "xBGR";
	case JCS_EXT_RGBA:
		return "JCS_EXT_RGBA";
	case JCS_EXT_BGRA:
		return "RGBA";
	case JCS_EXT_ABGR:
		return "ARGB";
	case JCS_RGB565:
		return "RGB565";
	}
	return "Unknown";
}


FrameWriterUnit* FileJPEG::new_writer_unit(FrameWriter *writer)
{
	return new JPEGUnit(this, writer);
}


JPEGUnit::JPEGUnit(FileJPEG *file, FrameWriter *writer)
 : FrameWriterUnit(writer)
{
	temp_frame = 0;
	compressed = 0;
}

JPEGUnit::~JPEGUnit()
{
	delete temp_frame;
	free(compressed);
}

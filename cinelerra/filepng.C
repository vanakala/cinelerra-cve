// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "file.h"
#include "filepng.h"
#include "interlacemodes.h"
#include "language.h"
#include "mwindow.h"
#include "paramlist.h"
#include "paramlistwindow.h"
#include "tmpframecache.h"
#include "vframe.h"
#include "mainerror.h"

#include <png.h>
#include <string.h>

#define FILEPNG_VCODEC_IX 0
#define PNG_ENC_CONFIG_NAME "png:enc"

#define PARAM_ALPHA "use_alpha"

struct paramlist_defaults FilePNG::encoder_params[] =
{
	{ PARAM_ALPHA, N_("Use alpha"), PARAMTYPE_INT | PARAMTYPE_BOOL, 0 },
	{ 0, 0, 0, 0 }
};

FilePNG::FilePNG(Asset *asset, File *file)
 : FileList(asset, file, "PNGLIST", ".png", FILE_PNG, FILE_PNG_LIST)
{
	native_cmodel = BC_RGB888;
}

int FilePNG::check_sig(Asset *asset)
{
	FILE *stream = fopen(asset->path, "rb");

	if(stream)
	{
		char test[16];

		int l = fread(test, 16, 1, stream);
		fclose(stream);
		if(!l)
			return 0;

		if(png_check_sig((png_const_bytep)test, 8))
			return 1;
		else
		if(!strncmp("PNGLIST", test, 7))
			return 1;
	}
	return 0;
}

void FilePNG::get_parameters(BC_WindowBase *parent_window, 
	Asset *asset, 
	BC_WindowBase* &format_window,
	int options)
{
	Paramlist *plist;

	if(options & SUPPORTS_VIDEO)
	{
		asset->encoder_parameters[FILEPNG_VCODEC_IX] =
			Paramlist::construct("FilePNG",
				asset->encoder_parameters[FILEPNG_VCODEC_IX],
				encoder_params);

		plist = Paramlist::clone(asset->encoder_parameters[FILEPNG_VCODEC_IX]);

		ParamlistThread thread(&plist, _("PNG compression"),
			mwindow_global->get_window_icon(), &format_window);
		thread.run();

		if(!thread.win_result)
		{
			if(!plist->equiv(asset->encoder_parameters[FILEPNG_VCODEC_IX]))
				asset->encoder_parameters[FILEPNG_VCODEC_IX]->copy_values(plist);
		}
		delete plist;
	}
}

int FilePNG::read_frame_header(const char *path)
{
	int result = 0;
	int color_type;
	int color_depth;
	int num_trans = 0;

	FILE *stream;

	if(!(stream = fopen(path, "rb")))
	{
		errormsg("Error while opening \"%s\" for reading. \n%m", asset->path);
		return 1;
	}

	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info = 0;	
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	info_ptr = png_create_info_struct(png_ptr);
	png_init_io(png_ptr, stream);

	png_read_info(png_ptr, info_ptr);

	asset->interlace_mode = BC_ILACE_MODE_NOTINTERLACED;

	color_type = png_get_color_type(png_ptr, info_ptr);
	color_depth = png_get_bit_depth(png_ptr,info_ptr);

	png_get_tRNS(png_ptr, info_ptr, NULL, &num_trans, NULL);

	if(color_depth == 16)
	{
		if (color_type & PNG_COLOR_MASK_ALPHA)
			native_cmodel = BC_RGBA16161616;
		else
			native_cmodel = BC_RGB161616;
	}
	else if((color_type & PNG_COLOR_MASK_ALPHA) || (num_trans > 0))
		native_cmodel = BC_RGBA8888;
	else
		native_cmodel = BC_RGB888;

	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	fclose(stream);

	return result;
}

static void read_function(png_structp png_ptr,
	png_bytep data, 
	png_uint_32 length)
{
	VFrame *input = (VFrame*)png_get_io_ptr(png_ptr);

	memcpy(data, input->get_data() + input->get_compressed_size(), length);
	input->set_compressed_size(input->get_compressed_size() + length);
}

static void write_function(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
	VFrame *output = (VFrame*)png_get_io_ptr(png_ptr);

	if(output->get_compressed_allocated() < output->get_compressed_size() + length)
		output->allocate_compressed_data((output->get_compressed_allocated() + length) * 2);
	memcpy(output->get_data() + output->get_compressed_size(), data, length);
	output->set_compressed_size(output->get_compressed_size() + length);
}

static void flush_function(png_structp png_ptr)
{
	;
}

int FilePNG::write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit)
{
	PNGUnit *png_unit = (PNGUnit*)unit;
	int result = 0;
	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info = 0;
	VFrame *output_frame;
	int use_alpha = 0;
	int stream = asset->get_stream_ix(STRDSC_VIDEO);

	if(asset->encoder_parameters[FILEPNG_VCODEC_IX])
		use_alpha = asset->encoder_parameters[FILEPNG_VCODEC_IX]->get(PARAM_ALPHA, 0);

	data->set_compressed_size(0);

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	info_ptr = png_create_info_struct(png_ptr);
	png_set_write_fn(png_ptr,
			data,
			(png_rw_ptr)write_function,
			(png_flush_ptr)flush_function);
	png_set_compression_level(png_ptr, 9);

	png_set_IHDR(png_ptr, 
		info_ptr, 
		asset->streams[stream].width,
		asset->streams[stream].height,
		8,
		use_alpha ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE, 
		PNG_COMPRESSION_TYPE_DEFAULT, 
		PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);

	native_cmodel = use_alpha ? BC_RGBA8888 : BC_RGB888;
	if(frame->get_color_model() != native_cmodel)
	{
		if(!png_unit->temp_frame)
			png_unit->temp_frame = new VFrame(0,
				asset->streams[stream].width,
				asset->streams[stream].height,
				native_cmodel);
		png_unit->temp_frame->transfer_from(frame);
		output_frame = png_unit->temp_frame;
	}
	else
		output_frame = frame;

	png_write_image(png_ptr, output_frame->get_rows());
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	if(output_frame == frame)
		frame->delete_row_ptrs();
	return result;
}

int FilePNG::read_frame(VFrame *output, VFrame *input)
{
	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info = 0;
	int result = 0;
	int frame_cmodel;
	int color_type;
	int color_depth;
	VFrame *work_frame;
	int size = input->get_compressed_size();
	input->set_compressed_size(0);

	output->clear_frame();
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	info_ptr = png_create_info_struct(png_ptr);
	png_set_read_fn(png_ptr, input, (png_rw_ptr)read_function);
	png_read_info(png_ptr, info_ptr);

	color_type = png_get_color_type(png_ptr, info_ptr);
	if(color_type == PNG_COLOR_TYPE_GRAY ||
			color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);

	color_depth = png_get_bit_depth(png_ptr, info_ptr);

	if(color_depth <= 8)
	{
		png_set_expand(png_ptr);
		frame_cmodel = color_type & PNG_COLOR_MASK_ALPHA ?
			BC_RGBA8888 : BC_RGB888;
	}
	else
	{
		frame_cmodel = color_type & PNG_COLOR_MASK_ALPHA ?
			BC_RGBA16161616 : BC_RGB161616;
		png_set_swap(png_ptr);
	}

	work_frame = BC_Resources::tmpframes.get_tmpframe(
		png_get_image_width(png_ptr, info_ptr),
		png_get_image_height(png_ptr, info_ptr),
		frame_cmodel, "FilePNG::read_frame");

	if(!(color_type & PNG_COLOR_MASK_COLOR))
		png_set_gray_to_rgb(png_ptr);

	if(color_type & PNG_COLOR_MASK_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	png_read_image(png_ptr, work_frame->get_rows());
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

	input->set_compressed_size(size);

	output->transfer_from(work_frame);
	BC_Resources::tmpframes.release_frame(work_frame);

	if(ColorModels::has_alpha(frame_cmodel))
		output->set_transparent();

	return result;
}

void FilePNG::save_render_options(Asset *asset)
{
	char pathbuf[BCTEXTLEN];

	asset->profile_config_path(PNG_ENC_CONFIG_NAME, pathbuf);
	strcat(pathbuf, XML_CONFIG_EXT);

	Paramlist::save_paramlist(asset->encoder_parameters[FILEPNG_VCODEC_IX],
		pathbuf, encoder_params);
}

void FilePNG::load_render_options(Asset *asset)
{
	char pathbuf[BCTEXTLEN];

	asset->profile_config_path(PNG_ENC_CONFIG_NAME, pathbuf);
	strcat(pathbuf, XML_CONFIG_EXT);

	delete asset->encoder_parameters[FILEPNG_VCODEC_IX];
	asset->encoder_parameters[FILEPNG_VCODEC_IX] =
		Paramlist::load_paramlist(pathbuf);
}

FrameWriterUnit* FilePNG::new_writer_unit(FrameWriter *writer)
{
	return new PNGUnit(this, writer);
}


PNGUnit::PNGUnit(FilePNG *file, FrameWriter *writer)
 : FrameWriterUnit(writer)
{
	this->file = file;
	temp_frame = 0;
}

PNGUnit::~PNGUnit()
{
	delete temp_frame;
}

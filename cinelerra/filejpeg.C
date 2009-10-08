
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "asset.h"
#include "bcsignals.h"
#include "edit.h"
#include "file.h"
#include "filejpeg.h"
#include "interlacemodes.h"
#include "jpegwrapper.h"
#include "language.h"
#include "libmjpeg.h"
#include "mwindow.inc"
#include "quicktime.h"
#include "vframe.h"
#include "videodevice.inc"
#include "mainerror.h"


FileJPEG::FileJPEG(Asset *asset, File *file)
 : FileList(asset, file, "JPEGLIST", ".jpg", FILE_JPEG, FILE_JPEG_LIST)
{
	decompressor = 0;
}

FileJPEG::~FileJPEG()
{
	if(decompressor) mjpeg_delete((mjpeg_t*)decompressor);
}


int FileJPEG::check_sig(Asset *asset)
{
	FILE *stream = fopen(asset->path, "rb");

	if(stream)
	{
		char test[10];
		fread(test, 10, 1, stream);
		fclose(stream);

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
		if(!strncasecmp(asset->path + len - 4, ".jpg", 4)) return 1;
	}
	return 0;
}



void FileJPEG::get_parameters(BC_WindowBase *parent_window, 
	Asset *asset, 
	BC_WindowBase* &format_window,
	int audio_options,
	int video_options)
{
	if(video_options)
	{
		JPEGConfigVideo *window = new JPEGConfigVideo(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
}


int FileJPEG::can_copy_from(Edit *edit, int64_t position)
{
//printf("FileJPEG::can_copy_from %d %s\n", asset->format, asset->vcodec);
	if(edit->asset->format == FILE_MOV)
	{
		if(match4(edit->asset->vcodec, QUICKTIME_JPEG)) return 1;
	}
	else
	if(edit->asset->format == FILE_JPEG || 
		edit->asset->format == FILE_JPEG_LIST)
		return 1;

	return 0;
}

int FileJPEG::colormodel_supported(int colormodel)
{
	return colormodel;
}


int FileJPEG::get_best_colormodel(Asset *asset, int driver)
{
	switch(driver)
	{
		case PLAYBACK_X11:
			return BC_RGB888;
			break;
		case PLAYBACK_X11_XV:
		case PLAYBACK_DV1394:
		case PLAYBACK_FIREWIRE:
		case PLAYBACK_ASYNCHRONOUS:
			return BC_YUV420P;
			break;
		case PLAYBACK_X11_GL:
			return BC_YUV888;
			break;
		case PLAYBACK_LML:
		case PLAYBACK_BUZ:
			return BC_YUV422P;
			break;
		case VIDEO4LINUX:
		case VIDEO4LINUX2:
			return BC_YUV420P;
			break;
		case CAPTURE_BUZ:
		case CAPTURE_LML:
		case VIDEO4LINUX2JPEG:
			return BC_YUV422;
			break;
		case CAPTURE_FIREWIRE:
		case CAPTURE_IEC61883:
			return BC_YUV420P;
			break;
	}
	return BC_YUV420P;
}


int FileJPEG::write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit)
{
	int result = 0;
	JPEGUnit *jpeg_unit = (JPEGUnit*)unit;

	if(!jpeg_unit->compressor)
		jpeg_unit->compressor = mjpeg_new(asset->width, 
			asset->height, 
			1);
	mjpeg_set_quality((mjpeg_t*)jpeg_unit->compressor, asset->jpeg_quality);
	
	
	mjpeg_compress((mjpeg_t*)jpeg_unit->compressor, 
		frame->get_rows(), 
		frame->get_y(), 
		frame->get_u(), 
		frame->get_v(),
		frame->get_color_model(),
		1);
	
	data->allocate_compressed_data(mjpeg_output_size((mjpeg_t*)jpeg_unit->compressor));
	data->set_compressed_size(mjpeg_output_size((mjpeg_t*)jpeg_unit->compressor));
	memcpy(data->get_data(), 
		mjpeg_output_buffer((mjpeg_t*)jpeg_unit->compressor), 
		mjpeg_output_size((mjpeg_t*)jpeg_unit->compressor));
	
	return result;
}










int FileJPEG::read_frame_header(char *path)
{
	int result = 0;


	FILE *stream;

	if(!(stream = fopen(path, "rb")))
	{
		eprintf("Error while opening \"%s\" for reading. \n%m\n", asset->path);
		return 1;
	}
	
	

	struct jpeg_decompress_struct jpeg_decompress;
	struct jpeg_error_mgr jpeg_error;

	jpeg_decompress.err = jpeg_std_error(&jpeg_error);
	jpeg_create_decompress(&jpeg_decompress);

	jpeg_stdio_src(&jpeg_decompress, stream);
	jpeg_read_header(&jpeg_decompress, TRUE);

	asset->width = jpeg_decompress.image_width;
	asset->height = jpeg_decompress.image_height;

	asset->interlace_mode = BC_ILACE_MODE_NOTINTERLACED;

	jpeg_destroy((j_common_ptr)&jpeg_decompress);
	fclose(stream);
	
	
	
	return result;
}



int FileJPEG::read_frame(VFrame *output, VFrame *input)
{
	if(!decompressor) decompressor = mjpeg_new(asset->width, 
		asset->height, 
		1);
	mjpeg_decompress((mjpeg_t*)decompressor, 
		input->get_data(), 
		input->get_compressed_size(),
		0,  
		output->get_rows(), 
		output->get_y(), 
		output->get_u(), 
		output->get_v(),
		output->get_color_model(),
		1);


	return 0;
}

FrameWriterUnit* FileJPEG::new_writer_unit(FrameWriter *writer)
{
	return new JPEGUnit(this, writer);
}






JPEGUnit::JPEGUnit(FileJPEG *file, FrameWriter *writer)
 : FrameWriterUnit(writer)
{
	this->file = file;
	compressor = 0;
}
JPEGUnit::~JPEGUnit()
{
	if(compressor) mjpeg_delete((mjpeg_t*)compressor);
}







JPEGConfigVideo::JPEGConfigVideo(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Video Compression",
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	400,
	100)
{
	this->parent_window = parent_window;
	this->asset = asset;
}

JPEGConfigVideo::~JPEGConfigVideo()
{
}

int JPEGConfigVideo::create_objects()
{
	int x = 10, y = 10;

	add_subwindow(new BC_Title(x, y, _("Quality:")));
	add_subwindow(new BC_ISlider(x + 80, 
		y,
		0,
		200,
		200,
		0,
		100,
		asset->jpeg_quality,
		0,
		0,
		&asset->jpeg_quality));

	add_subwindow(new BC_OKButton(this));
	return 0;
}

int JPEGConfigVideo::close_event()
{
	set_done(0);
	return 1;
}




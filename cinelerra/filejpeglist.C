
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

#include "assets.h"
#include "file.h"
#include "filejpeglist.h"
#include "guicast.h"
#include "libmjpeg.h"
#include "mwindow.inc"
#include "jpegwrapper.h"
#include "quicktime.h"
#include "vframe.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>


FileJPEGList::FileJPEGList(Asset *asset, File *file)
 : FileList(asset, file)
{
	asset->format = FILE_JPEG_LIST;
}

FileJPEGList::~FileJPEGList()
{
	close_file();
}

char* FileJPEGList::list_title()
{
	return _("JPEGLIST");
}

char* FileJPEGList::extension()
{
	return ".jpg";
}

void FileJPEGList::get_parameters(BC_WindowBase *parent_window, 
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

int FileJPEGList::can_copy_from(Asset *asset)
{
	if(asset->format == FILE_JPEG_LIST)
		return 1;
	else
	if(asset->format == FILE_MOV && match4(asset->vcodec, QUICKTIME_JPEG))
		return 1;
	
	return 0;
}

int FileJPEGList::get_best_colormodel(int driver, int colormodel)
{
	if(colormodel > -1)
	{
		switch(colormodel)
		{
			case BC_YUV420P:
			case BC_YUV422P:
			case BC_YUV422:
				return colormodel;
				break;
			default:
				return BC_YUV422P;
				break;
		}
	}
	return BC_YUV422P;
}


int FileJPEGList::read_frame(VFrame *frame, VFrame *data)
{
	mjpeg_t *mjpeg = mjpeg_new(asset->width, 
		asset->height, 
		1);
	mjpeg_decompress(mjpeg, 
		data->get_data(), 
		data->get_compressed_size(),
		0,  
		frame->get_rows(), 
		frame->get_y(), 
		frame->get_u(), 
		frame->get_v(),
		frame->get_color_model(),
		file->cpus);
	mjpeg_delete(mjpeg);
	return 0;
}

int FileJPEGList::write_frame(VFrame *frame, VFrame *data)
{
	mjpeg_t *mjpeg = mjpeg_new(asset->width, 
		asset->height, 
		1);
	mjpeg_compress(mjpeg, 
		frame->get_rows(), 
		frame->get_y(), 
		frame->get_u(), 
		frame->get_v(),
		frame->get_color_model(),
		file->cpus);
	data->allocate_compressed_data(mjpeg_output_size(mjpeg));
	bcopy(mjpeg_output_buffer(mjpeg), frame->get_data(), mjpeg_output_size(mjpeg));
	mjpeg_delete(mjpeg);
	return 0;
}











JPEGConfigVideo::JPEGConfigVideo(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Video Compression",
 	parent_window->get_abs_cursor_x(),
 	parent_window->get_abs_cursor_y(),
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
		asset->quality,
		0,
		0,
		&asset->quality));

	add_subwindow(new BC_OKButton(this));
	return 0;
}

int JPEGConfigVideo::close_event()
{
	set_done(0);
	return 1;
}








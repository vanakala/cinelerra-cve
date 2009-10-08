
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
#include "edit.h"
#include "file.h"
#include "filepng.h"
#include "interlacemodes.h"
#include "language.h"
#include "mwindow.inc"
#include "quicktime.h"
#include "vframe.h"
#include "videodevice.inc"
#include "mainerror.h"

#include <png.h>

FilePNG::FilePNG(Asset *asset, File *file)
 : FileList(asset, file, "PNGLIST", ".png", FILE_PNG, FILE_PNG_LIST)
{
}

FilePNG::~FilePNG()
{
}



int FilePNG::check_sig(Asset *asset)
{
	FILE *stream = fopen(asset->path, "rb");

	if(stream)
	{

//printf("FilePNG::check_sig 1\n");
		char test[16];
		fread(test, 16, 1, stream);
		fclose(stream);

		if(png_check_sig((unsigned char*)test, 8))
		{
//printf("FilePNG::check_sig 1\n");
			return 1;
		}
		else
		if(test[0] == 'P' && test[1] == 'N' && test[2] == 'G' && 
			test[3] == 'L' && test[4] == 'I' && test[5] == 'S' && test[6] == 'T')
		{
//printf("FilePNG::check_sig 1\n");
			return 1;
		}
	}
	return 0;
}



void FilePNG::get_parameters(BC_WindowBase *parent_window, 
	Asset *asset, 
	BC_WindowBase* &format_window,
	int audio_options,
	int video_options)
{
	if(video_options)
	{
		PNGConfigVideo *window = new PNGConfigVideo(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
}




int FilePNG::can_copy_from(Edit *edit, int64_t position)
{
	if(edit->asset->format == FILE_MOV)
	{
		if(match4(edit->asset->vcodec, QUICKTIME_PNG)) return 1;
	}
	else
	if(edit->asset->format == FILE_PNG || 
		edit->asset->format == FILE_PNG_LIST)
		return 1;

	return 0;
}


int FilePNG::colormodel_supported(int colormodel)
{
	if (((colormodel == BC_RGBA8888) && (native_cmodel == BC_RGBA16161616))
	    || ((colormodel == BC_RGB161616) && (native_cmodel == BC_RGBA16161616))
	    || (colormodel == BC_RGB888))
	{
	    return colormodel;
	}
	else if ((colormodel == BC_RGB161616) && (native_cmodel == BC_RGBA8888))
	{
	    return BC_RGB888;
	}
	else
	{
	    return native_cmodel;
	}
}


int FilePNG::get_best_colormodel(Asset *asset, int driver)
{
	if(asset->png_use_alpha)
		return BC_RGBA8888;
	else
		return BC_RGB888;
}




int FilePNG::read_frame_header(char *path)
{
	int result = 0;
	int color_type;
	int color_depth;
	int num_trans = 0;
	
	FILE *stream;

	if(!(stream = fopen(path, "rb")))
	{
		eprintf("Error while opening \"%s\" for reading. \n%m\n", asset->path);
		return 1;
	}

	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info = 0;	
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	info_ptr = png_create_info_struct(png_ptr);
	png_init_io(png_ptr, stream);


	png_read_info(png_ptr, info_ptr);

	asset->width = png_get_image_width(png_ptr, info_ptr);
	asset->height = png_get_image_height(png_ptr, info_ptr);
	
	asset->interlace_mode = BC_ILACE_MODE_NOTINTERLACED;

	color_type = png_get_color_type(png_ptr, info_ptr);
	color_depth = png_get_bit_depth(png_ptr,info_ptr);
	
	png_get_tRNS(png_ptr, info_ptr, NULL, &num_trans, NULL);
	
	if (color_depth == 16)
	{
	    if (color_type & PNG_COLOR_MASK_ALPHA)
	    {
	        native_cmodel = BC_RGBA16161616;
	    }
	    else
	    {
	        native_cmodel = BC_RGB161616;
	    }
	}
	else 
	if ((color_type & PNG_COLOR_MASK_ALPHA)
	    || (num_trans > 0))
	{
	    native_cmodel = BC_RGBA8888;
	}
	else
	{
	    native_cmodel = BC_RGB888;
	}


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
	data->set_compressed_size(0);

//printf("FilePNG::write_frame 1\n");
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	info_ptr = png_create_info_struct(png_ptr);
	png_set_write_fn(png_ptr,
               data, 
			   (png_rw_ptr)write_function,
               (png_flush_ptr)flush_function);
	png_set_compression_level(png_ptr, 9);

	png_set_IHDR(png_ptr, 
		info_ptr, 
		asset->width, 
		asset->height,
    	8, 
		asset->png_use_alpha ? 
		  PNG_COLOR_TYPE_RGB_ALPHA : 
		  PNG_COLOR_TYPE_RGB, 
		PNG_INTERLACE_NONE, 
		PNG_COMPRESSION_TYPE_DEFAULT, 
		PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);

//printf("FilePNG::write_frame 1\n");
	native_cmodel = asset->png_use_alpha ? BC_RGBA8888 : BC_RGB888;
	if(frame->get_color_model() != native_cmodel)
	{
		if(!png_unit->temp_frame) png_unit->temp_frame = new VFrame(0, 
			asset->width, 
			asset->height, 
			native_cmodel);

		cmodel_transfer(png_unit->temp_frame->get_rows(), /* Leave NULL if non existent */
			frame->get_rows(),
			png_unit->temp_frame->get_y(), /* Leave NULL if non existent */
			png_unit->temp_frame->get_u(),
			png_unit->temp_frame->get_v(),
			frame->get_y(), /* Leave NULL if non existent */
			frame->get_u(),
			frame->get_v(),
			0,        /* Dimensions to capture from input frame */
			0, 
			asset->width, 
			asset->height,
			0,       /* Dimensions to project on output frame */
			0, 
			asset->width, 
			asset->height,
			frame->get_color_model(), 
			png_unit->temp_frame->get_color_model(),
			0,         /* When transfering BC_RGBA8888 to non-alpha this is the background color in 0xRRGGBB hex */
			asset->width,       /* For planar use the luma rowspan */
			asset->height);
		
		output_frame = png_unit->temp_frame;
	}
	else
		output_frame = frame;


//printf("FilePNG::write_frame 2\n");
	png_write_image(png_ptr, output_frame->get_rows());
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
//printf("FilePNG::write_frame 3 %d\n", data->get_compressed_size());

	return result;
}

int FilePNG::read_frame(VFrame *output, VFrame *input)
{
	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info = 0;	
	int result = 0;
	int color_type;
	int color_depth;
	int colormodel;
	int size = input->get_compressed_size();
	input->set_compressed_size(0);
	
	
	//printf("FilePNG::read_frame 1 %d %d\n", native_cmodel, output->get_color_model());
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	info_ptr = png_create_info_struct(png_ptr);
	png_set_read_fn(png_ptr, input, (png_rw_ptr)read_function);
	png_read_info(png_ptr, info_ptr);

 	int png_color_type = png_get_color_type(png_ptr, info_ptr);
 	if (png_color_type == PNG_COLOR_TYPE_GRAY ||
         	png_color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
 	{
 		png_set_gray_to_rgb(png_ptr);
 	}

	colormodel = output->get_color_model();
	color_type = png_get_color_type(png_ptr, info_ptr);
	color_depth = png_get_bit_depth(png_ptr,info_ptr);
	
	if (((native_cmodel == BC_RGBA16161616)||(native_cmodel == BC_RGB161616))
	    && ((colormodel == BC_RGBA8888)||(colormodel == BC_RGB888)))
	{
	    png_set_strip_16(png_ptr);
	}

	/* If we're dropping the alpha channel, use the background color of the image
	   otherwise, use black */
	if (((native_cmodel == BC_RGBA16161616)||(native_cmodel == BC_RGBA8888))
	    && ((colormodel == BC_RGB161616)||(colormodel == BC_RGB888)))
	{
	    png_color_16 my_background;
	    png_color_16p image_background;
	    
	    memset(&my_background,0,sizeof(png_color_16));
	    
	    if (png_get_bKGD(png_ptr, info_ptr, &image_background))
	    {
	        png_set_background(png_ptr, image_background, PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
	    }
	    else
	    {
	        png_set_background(png_ptr, &my_background, PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
	    }
	}
	
	/* Little endian */
	if ((color_depth == 16)
	    &&((colormodel == BC_RGBA16161616)||(colormodel == BC_RGB161616)))
	{
	    png_set_swap(png_ptr);
	}
	
	if (!(color_type & PNG_COLOR_MASK_COLOR))
	{
	    png_set_gray_to_rgb(png_ptr);
	}
	
	if (color_type & PNG_COLOR_MASK_PALETTE)
	{
	    png_set_palette_to_rgb(png_ptr);
	}
	
	if (color_depth <= 8)
	{
	    png_set_expand(png_ptr);
	}

/* read the image */
	png_read_image(png_ptr, output->get_rows());
//printf("FilePNG::read_frame 3\n");
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

	input->set_compressed_size(size);

//printf("FilePNG::read_frame 4\n");
	return result;
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
	if(temp_frame) delete temp_frame;
}









PNGConfigVideo::PNGConfigVideo(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Video Compression",
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	200,
	100)
{
	this->parent_window = parent_window;
	this->asset = asset;
}

PNGConfigVideo::~PNGConfigVideo()
{
}

int PNGConfigVideo::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new PNGUseAlpha(this, x, y));
	add_subwindow(new BC_OKButton(this));
	return 0;
}

int PNGConfigVideo::close_event()
{
	set_done(0);
	return 1;
}


PNGUseAlpha::PNGUseAlpha(PNGConfigVideo *gui, int x, int y)
 : BC_CheckBox(x, y, gui->asset->png_use_alpha, _("Use alpha"))
{
	this->gui = gui;
}

int PNGUseAlpha::handle_event()
{
	gui->asset->png_use_alpha = get_value();
	return 1;
}




#include "assets.h"
#include "bccapture.h"
#include "canvas.h"
#include "colormodels.h"
#include "mwindow.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "recordconfig.h"
#include "strategies.inc"
#include "vdevicex11.h"
#include "vframe.h"
#include "videodevice.h"
#include "videowindow.h"
#include "videowindowgui.h"

#include <string.h>
#include <unistd.h>

VDeviceX11::VDeviceX11(VideoDevice *device, Canvas *output)
 : VDeviceBase(device)
{
	reset_parameters();
	this->output = output;
}

VDeviceX11::~VDeviceX11()
{
	close_all();
}

int VDeviceX11::reset_parameters()
{
	output_frame = 0;
	bitmap = 0;
	bitmap_w = 0;
	bitmap_h = 0;
	output = 0;
	in_x = 0;
	in_y = 0;
	in_w = 0;
	in_h = 0;
	out_x = 0;
	out_y = 0;
	out_w = 0;
	out_h = 0;
	capture_bitmap = 0;
	color_model_selected = 0;
	return 0;
}

int VDeviceX11::open_input()
{
//printf("VDeviceX11::open_input 1\n");
	capture_bitmap = new BC_Capture(device->in_config->w, 
		device->in_config->h,
		device->in_config->screencapture_display);
//printf("VDeviceX11::open_input 2\n");
	
	return 0;
}

int VDeviceX11::open_output()
{
	if(output && !device->single_frame) 
		output->canvas->start_video();
	return 0;
}


int VDeviceX11::output_visible()
{
	if(!output || output->canvas->get_hidden()) 
		return 0; 
	else 
		return 1;
}


int VDeviceX11::close_all()
{
//printf("VDeviceX11::close_all 1\n");
	if(output)
	{
		output->canvas->lock_window();
	}

//printf("VDeviceX11::close_all 2\n");
	if(output && output_frame)
	{
// Copy picture to persistent frame buffer with conversion to flat colormodel
		if(output->refresh_frame &&
			(output->refresh_frame->get_w() != device->out_w ||
			output->refresh_frame->get_h() != device->out_h ||
			output->refresh_frame->get_color_model() != output_frame->get_color_model()))
		{
			delete output->refresh_frame;
			output->refresh_frame = 0;
		}

//printf("VDeviceX11::close_all 3\n");
		if(!output->refresh_frame)
		{
			output->refresh_frame = new VFrame(0,
				device->out_w,
				device->out_h,
				output_frame->get_color_model());
		}

//printf("VDeviceX11::close_all 4\n");
		if(!device->single_frame)
		{
//printf("VDeviceX11::close_all 5\n");
			output->canvas->stop_video();
		}

//printf("VDeviceX11::close_all 5\n");
		output->refresh_frame->copy_from(output_frame);

//printf("VDeviceX11::close_all 6\n");
		output->draw_refresh();
	}

//printf("VDeviceX11::close_all 8\n");
	if(bitmap)
	{
		delete bitmap;
		delete output_frame;
		bitmap = 0;
	}

//printf("VDeviceX11::close_all 0\n");
	if(capture_bitmap) delete capture_bitmap;

//printf("VDeviceX11::close_all 10\n");
	if(output)
	{
		output->canvas->unlock_window();
	}

//printf("VDeviceX11::close_all 11\n");

	reset_parameters();
//printf("VDeviceX11::close_all 12\n");
	return 0;
}

int VDeviceX11::read_buffer(VFrame *frame)
{
//printf("VDeviceX11::read_buffer 1\n");
	capture_bitmap->capture_frame(frame, device->input_x, device->input_y);
//printf("VDeviceX11::read_buffer 2\n");
	return 0;
}


int VDeviceX11::get_best_colormodel(Asset *asset)
{
	return BC_RGB888;
}


int VDeviceX11::get_best_colormodel(int colormodel)
{
	int result = -1;
	if(!device->single_frame)
	{
		switch(colormodel)
		{
			case BC_YUV420P:
			case BC_YUV422P:
			case BC_YUV422:
				result = colormodel;
				break;
		}
	}

	if(result < 0)
	{
		switch(colormodel)
		{
			case BC_RGB888:
			case BC_RGBA8888:
			case BC_RGB161616:
			case BC_RGBA16161616:
			case BC_YUV888:
			case BC_YUVA8888:
			case BC_YUV161616:
			case BC_YUVA16161616:
				result = colormodel;
				break;

			default:
				result = output->canvas->get_color_model();
				break;
		}
	}

//printf("VDeviceX11::get_best_colormodel %d %d %d\n", device->single_frame, colormodel, result);
	return result;
}


void VDeviceX11::new_output_buffer(VFrame **output_channels, int colormodel)
{
	for(int i = 0; i < MAX_CHANNELS; i++)
		output_channels[i] = 0;
//printf("VDeviceX11::new_output_buffer 1\n");

// Get the best colormodel the display can handle.
	int best_colormodel = get_best_colormodel(colormodel);
//printf("VDeviceX11::new_output_buffer 2 %d\n", best_colormodel);

// Conform existing bitmap to new colormodel and output size
	if(bitmap)
	{
// Restart if output size changed or output colormodel changed
		if(!color_model_selected ||
			(!bitmap->hardware_scaling() && 
				(bitmap->get_w() != output->canvas->get_w() ||
				bitmap->get_h() != output->canvas->get_h())) ||
			colormodel != output_frame->get_color_model())
		{
			int size_change = (bitmap->get_w() != output->canvas->get_w() ||
				bitmap->get_h() != output->canvas->get_h());
			delete bitmap;
			delete output_frame;
			bitmap = 0;
			output_frame = 0;

// Blank only if size changed
			if(size_change)
			{
				output->canvas->set_color(BLACK);
				output->canvas->draw_box(0, 0, output->w, output->h);
				output->canvas->flash();
			}
		}
		else
// Update the ring buffer
		if(bitmap_type == BITMAP_PRIMARY)
		{

//printf("VDeviceX11::new_output_buffer\n");
			output_frame->set_memory((unsigned char*)bitmap->get_data() /* + bitmap->get_shm_offset() */,
						bitmap->get_y_offset(),
						bitmap->get_u_offset(),
						bitmap->get_v_offset());
		}
	}

// Create new bitmap
	if(!bitmap)
	{
//printf("VDeviceX11::new_output_buffer 1 %d\n", best_colormodel);
// Try hardware accelerated
		switch(best_colormodel)
		{
			case BC_YUV420P:
				if(device->out_config->driver == PLAYBACK_X11_XV &&
					output->canvas->accel_available(best_colormodel) &&
					!output->use_scrollbars)
				{
					bitmap = new BC_Bitmap(output->canvas, 
						device->out_w,
						device->out_h,
						best_colormodel,
						1);
					output_frame = new VFrame((unsigned char*)bitmap->get_data() + bitmap->get_shm_offset(), 
						bitmap->get_y_offset(),
						bitmap->get_u_offset(),
						bitmap->get_v_offset(),
						device->out_w,
						device->out_h,
						best_colormodel);
					bitmap_type = BITMAP_PRIMARY;
				}
				break;

			case BC_YUV422P:
				if(device->out_config->driver == PLAYBACK_X11_XV &&
					output->canvas->accel_available(best_colormodel) &&
					!output->use_scrollbars)
				{
					bitmap = new BC_Bitmap(output->canvas, 
						device->out_w,
						device->out_h,
						best_colormodel,
						1);
					output_frame = new VFrame((unsigned char*)bitmap->get_data() + bitmap->get_shm_offset(), 
						bitmap->get_y_offset(),
						bitmap->get_u_offset(),
						bitmap->get_v_offset(),
						device->out_w,
						device->out_h,
						best_colormodel);
					bitmap_type = BITMAP_PRIMARY;
				}
				else
				if(device->out_config->driver == PLAYBACK_X11_XV &&
					output->canvas->accel_available(BC_YUV422))
				{
					bitmap = new BC_Bitmap(output->canvas, 
						device->out_w,
						device->out_h,
						BC_YUV422,
						1);
					bitmap_type = BITMAP_TEMP;
				}
				break;

			case BC_YUV422:
//printf("VDeviceX11::new_output_buffer 3\n");
				if(device->out_config->driver == PLAYBACK_X11_XV &&
					output->canvas->accel_available(best_colormodel) &&
					!output->use_scrollbars)
				{
//printf("VDeviceX11::new_output_buffer 4\n");
					bitmap = new BC_Bitmap(output->canvas, 
						device->out_w,
						device->out_h,
						best_colormodel,
						1);
					output_frame = new VFrame((unsigned char*)bitmap->get_data() + bitmap->get_shm_offset(), 
						bitmap->get_y_offset(),
						bitmap->get_u_offset(),
						bitmap->get_v_offset(),
						device->out_w,
						device->out_h,
						best_colormodel);
					bitmap_type = BITMAP_PRIMARY;
				}
				else
				if(device->out_config->driver == PLAYBACK_X11_XV &&
					output->canvas->accel_available(BC_YUV422P))
				{
//printf("VDeviceX11::new_output_buffer 5\n");
					bitmap = new BC_Bitmap(output->canvas, 
						device->out_w,
						device->out_h,
						BC_YUV422P,
						1);
//printf("VDeviceX11::new_output_buffer 6\n");
					bitmap_type = BITMAP_TEMP;
//printf("VDeviceX11::new_output_buffer 7\n");
				}
				break;
		}
//printf("VDeviceX11::new_output_buffer 8\n");

// Try default colormodel
		if(!bitmap)
		{
			best_colormodel = output->canvas->get_color_model();
			bitmap = new BC_Bitmap(output->canvas, 
				output->canvas->get_w(),
				output->canvas->get_h(),
				best_colormodel,
				1);
			bitmap_type = BITMAP_TEMP;
//printf("VDeviceX11::new_output_buffer 9\n");
		}

		if(bitmap_type == BITMAP_TEMP)
		{
// Intermediate frame
			output_frame = new VFrame(0, 
				device->out_w,
				device->out_h,
				colormodel);
			bitmap_type = BITMAP_TEMP;
//printf("VDeviceX11::new_output_buffer 10\n");
		}
		color_model_selected = 1;
	}
//printf("VDeviceX11::new_output_buffer 11\n");

// Fill arguments
	if(bitmap_type == BITMAP_PRIMARY)
	{
// Only useful if the primary is RGB888 which XFree86 never uses.
		output_frame->set_shm_offset(bitmap->get_shm_offset());
	}
	else
	if(bitmap_type == BITMAP_TEMP)
	{
		output_frame->set_shm_offset(0);
	}
//printf("VDeviceX11::new_output_buffer 12\n");

// Temporary until multichannel X
	output_channels[0] = output_frame;
}


int VDeviceX11::start_playback()
{
// Record window is initialized when its monitor starts.
	if(!device->single_frame)
		output->canvas->start_video();
	return 0;
}

int VDeviceX11::stop_playback()
{
	if(!device->single_frame)
		output->canvas->stop_video();
// Record window goes back to monitoring
// get the last frame played and store it in the video_out
	return 0;
}

int VDeviceX11::write_buffer(VFrame **output_channels, EDL *edl)
{
	int i = 0;

// printf("VDeviceX11::write_buffer 1 %d %d\n",
// 	bitmap->hardware_scaling(), 
// 	output_channels[0]->get_color_model());

	output->canvas->lock_window();
	output->get_transfers(edl, 
		in_x, 
		in_y, 
		in_w, 
		in_h, 
		out_x, 
		out_y, 
		out_w, 
		out_h,
		(bitmap_type == BITMAP_TEMP && !bitmap->hardware_scaling()) ? bitmap->get_w() : -1,
		(bitmap_type == BITMAP_TEMP && !bitmap->hardware_scaling()) ? bitmap->get_h() : -1);

//output->canvas->unlock_window();
//return 0;
// printf("VDeviceX11::write_buffer 2 %d\n", bitmap_type);
// for(int j = 0; j < output_channels[0]->get_w() * 3 * 5; j++)
// 	output_channels[0]->get_rows()[0][j] = 255;

// Convert colormodel
	if(bitmap_type == BITMAP_TEMP)
	{
// printf("VDeviceX11::write_buffer 1 %d %d, %d %d %d %d -> %d %d %d %d\n",
// 			output->w,
// 			output->h,
// 			in_x, 
// 			in_y, 
// 			in_w, 
// 			in_h,
// 			out_x, 
// 			out_y, 
// 			out_w, 
// 			out_h );fflush(stdout);

//printf("VDeviceX11::write_buffer 2\n");

//printf("VDeviceX11::write_buffer 3 %p %p\n", bitmap->get_row_pointers(), output_channels[0]->get_rows());

		if(bitmap->hardware_scaling())
		{
			cmodel_transfer(bitmap->get_row_pointers(), 
				output_channels[0]->get_rows(),
				0,
				0,
				0,
				output_channels[0]->get_y(),
				output_channels[0]->get_u(),
				output_channels[0]->get_v(),
				0, 
				0, 
				output_channels[0]->get_w(), 
				output_channels[0]->get_h(),
				0, 
				0, 
				bitmap->get_w(), 
				bitmap->get_h(),
				output_channels[0]->get_color_model(), 
				bitmap->get_color_model(),
				0,
				output_channels[0]->get_w(),
				bitmap->get_w());
		}
		else
		{
			cmodel_transfer(bitmap->get_row_pointers(), 
				output_channels[0]->get_rows(),
				0,
				0,
				0,
				output_channels[0]->get_y(),
				output_channels[0]->get_u(),
				output_channels[0]->get_v(),
				in_x, 
				in_y, 
				in_w, 
				in_h,
				0, 
				0, 
				out_w, 
				out_h,
				output_channels[0]->get_color_model(), 
				bitmap->get_color_model(),
				0,
				output_channels[0]->get_w(),
				bitmap->get_w());
		}
	}

//printf("VDeviceX11::write_buffer 4 %p\n", bitmap);
//for(i = 0; i < 1000; i += 4) bitmap->get_data()[i] = 128;
//printf("VDeviceX11::write_buffer 2 %d %d %d\n", bitmap_type, 
//	bitmap->get_color_model(), 
//	output->get_color_model());fflush(stdout);
// printf("VDeviceX11::write_buffer 2 %d %d, %d %d %d %d -> %d %d %d %d\n",
// 			output->w,
// 			output->h,
// 			in_x, 
// 			in_y, 
// 			in_w, 
// 			in_h,
// 			out_x, 
// 			out_y, 
// 			out_w, 
// 			out_h);


// Select field if using field mode.  This may be a useful feature later on
// but currently it's being superceded by the heroine 60 encoding.
// Doing time base conversion in the display routine produces 
// pretty big compression artifacts.  It also requires implementing a
// different transform for each X visual.
	if(device->out_config->x11_use_fields)
	{
	}



// Cause X server to display it
	if(bitmap->hardware_scaling())
	{
		output->canvas->draw_bitmap(bitmap,
			!device->single_frame,
			out_x, 
			out_y, 
			out_w, 
			out_h,
			in_x, 
			in_y, 
			in_w, 
			in_h,
			0);
	}
	else
	{
		output->canvas->draw_bitmap(bitmap,
			!device->single_frame,
			out_x, 
			out_y, 
			out_w, 
			out_h,
			0, 
			0, 
			out_w, 
			out_h,
			0);
	}


// In single frame mode we want the frame to display once along with overlays
// when the device is closed.  This prevents intermediate drawing with overlays
// from reading the obsolete back buffer before the device is closed.

// 	if(device->single_frame)
// 	{
// 		output->canvas->flash();
// 		output->canvas->flush();
// 	}

	output->canvas->unlock_window();
//printf("VDeviceX11::write_buffer 5\n");fflush(stdout);
	return 0;
}



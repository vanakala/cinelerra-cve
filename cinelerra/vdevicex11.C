
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
#include "bccapture.h"
#include "bcsignals.h"
#include "canvas.h"
#include "colormodels.h"
#include "edl.h"
#include "edlsession.h"
#include "mwindow.h"
#include "playback3d.h"
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
	window_id = 0;
	bitmap = 0;
	bitmap_type = 0;
	bitmap_w = 0;
	bitmap_h = 0;
	output_x1 = 0;
	output_y1 = 0;
	output_x2 = 0;
	output_y2 = 0;
	canvas_x1 = 0;
	canvas_y1 = 0;
	canvas_x2 = 0;
	canvas_y2 = 0;
	capture_bitmap = 0;
	color_model_selected = 0;
	is_cleared = 0;
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
	if(output)
	{
		output->lock_canvas("VDeviceX11::open_output");
		output->get_canvas()->lock_window("VDeviceX11::open_output");
		if(!device->single_frame)
			output->start_video();
		else
			output->start_single();
		output->get_canvas()->unlock_window();

// Enable opengl in the first routine that needs it, to reduce the complexity.

		output->unlock_canvas();
	}
	return 0;
}


int VDeviceX11::output_visible()
{
	if(!output) return 0;

	output->lock_canvas("VDeviceX11::output_visible");
	if(output->get_canvas()->get_hidden()) 
	{
		output->unlock_canvas();
		return 0; 
	}
	else 
	{
		output->unlock_canvas();
		return 1;
	}
}


int VDeviceX11::close_all()
{
	if(output)
	{
		output->lock_canvas("VDeviceX11::close_all 1");
		output->get_canvas()->lock_window("VDeviceX11::close_all 1");
	}

	if(output && output_frame)
	{
// Copy our output frame buffer to the canvas's permanent frame buffer.
// They must be different buffers because the output frame is being written
// while the user is redrawing the canvas frame buffer over and over.

		int use_opengl = device->out_config->driver == PLAYBACK_X11_GL &&
			output_frame->get_opengl_state() == VFrame::SCREEN;
		int best_color_model = output_frame->get_color_model();

// OpenGL does YUV->RGB in the compositing step
		if(use_opengl)
			best_color_model = BC_RGB888;

		if(output->refresh_frame &&
			(output->refresh_frame->get_w() != device->out_w ||
			output->refresh_frame->get_h() != device->out_h ||
			output->refresh_frame->get_color_model() != best_color_model))
		{
			delete output->refresh_frame;
			output->refresh_frame = 0;
		}

		if(!output->refresh_frame)
		{
			output->refresh_frame = new VFrame(0,
				device->out_w,
				device->out_h,
				best_color_model);
		}

		if(use_opengl)
		{
			output->get_canvas()->unlock_window();
			output->unlock_canvas();

			output->mwindow->playback_3d->copy_from(output, 
				output->refresh_frame,
				output_frame,
				0);
			output->lock_canvas("VDeviceX11::close_all 2");
			output->get_canvas()->lock_window("VDeviceX11::close_all 2");
		}
		else
			output->refresh_frame->copy_from(output_frame);

// // Update the status bug
// 		if(!device->single_frame)
// 		{
// 			output->stop_video();
// 		}
// 		else
// 		{
// 			output->stop_single();
// 		}

// Draw the first refresh with new frame.
// Doesn't work if video and openGL because OpenGL doesn't have 
// the output buffer for video.
// Not necessary for any case if we mandate a frame advance after
// every stop.
		if(/* device->out_config->driver != PLAYBACK_X11_GL || 
			*/ device->single_frame)
			output->draw_refresh();
	}




	if(bitmap)
	{
		delete bitmap;
		bitmap = 0;
	}

	if(output_frame)
	{
		delete output_frame;
		output_frame = 0;
	}

	if(capture_bitmap) delete capture_bitmap;

	if(output)
	{
	
// Update the status bug
		if(!device->single_frame)
		{
			output->stop_video();
		}
		else
		{
			output->stop_single();
		}

		output->get_canvas()->unlock_window();
		output->unlock_canvas();
	}


	reset_parameters();
	return 0;
}

int VDeviceX11::read_buffer(VFrame *frame)
{
	capture_bitmap->capture_frame(frame, device->input_x, device->input_y);
	return 0;
}


int VDeviceX11::get_best_colormodel(Asset *asset)
{
	return BC_RGB888;
}


int VDeviceX11::get_best_colormodel(int colormodel)
{
	int result = -1;

	if(device->out_config->driver == PLAYBACK_X11_GL)
	{
		if(colormodel == BC_RGB888 ||
			colormodel == BC_RGBA8888 ||
			colormodel == BC_YUV888 ||
			colormodel == BC_YUVA8888 ||
			colormodel == BC_RGB_FLOAT ||
			colormodel == BC_RGBA_FLOAT)
		{
			return colormodel;
		}
		return BC_RGB888;
	}

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

// 2 more colormodels are supported by OpenGL
	if(device->out_config->driver == PLAYBACK_X11_GL)
	{
		if(colormodel == BC_RGB_FLOAT ||
			colormodel == BC_RGBA_FLOAT)
			result = colormodel;
	}

	if(result < 0)
	{
		switch(colormodel)
		{
			case BC_RGB888:
			case BC_RGBA8888:
			case BC_YUV888:
			case BC_YUVA8888:
				result = colormodel;
				break;

			default:
				output->lock_canvas("VDeviceX11::get_best_colormodel");
				result = output->get_canvas()->get_color_model();
				output->unlock_canvas();
				break;
		}
	}

	return result;
}


void VDeviceX11::new_output_buffer(VFrame **result, int colormodel)
{
//printf("VDeviceX11::new_output_buffer 1\n");
	output->lock_canvas("VDeviceX11::new_output_buffer");
	output->get_canvas()->lock_window("VDeviceX11::new_output_buffer 1");

// Get the best colormodel the display can handle.
	int best_colormodel = get_best_colormodel(colormodel);

// Only create OpenGL Pbuffer and texture.
	if(device->out_config->driver == PLAYBACK_X11_GL)
	{
// Create bitmap for initial load into texture.
// Not necessary to do through Playback3D.....yet
		if(!output_frame)
		{
			output_frame = new VFrame(0, 
				device->out_w, 
				device->out_h, 
				colormodel);
//BUFFER2(output_frame->get_rows()[0], "VDeviceX11::new_output_buffer 1");
		}

		window_id = output->get_canvas()->get_id();
		output_frame->set_opengl_state(VFrame::RAM);
	}
	else
	{
// Conform existing bitmap to new colormodel and output size
		if(bitmap)
		{
// Restart if output size changed or output colormodel changed.
// May have to recreate if transferring between windowed and fullscreen.
			if(!color_model_selected ||
				(!bitmap->hardware_scaling() && 
					(bitmap->get_w() != output->get_canvas()->get_w() ||
					bitmap->get_h() != output->get_canvas()->get_h())) ||
				colormodel != output_frame->get_color_model())
			{
				int size_change = (bitmap->get_w() != output->get_canvas()->get_w() ||
					bitmap->get_h() != output->get_canvas()->get_h());
				delete bitmap;
				delete output_frame;
				bitmap = 0;
				output_frame = 0;

// Blank only if size changed
				if(size_change)
				{
					output->get_canvas()->set_color(BLACK);
					output->get_canvas()->draw_box(0, 0, output->w, output->h);
					output->get_canvas()->flash();
				}
			}
			else
// Update the ring buffer
			if(bitmap_type == BITMAP_PRIMARY)
			{

				output_frame->set_memory((unsigned char*)bitmap->get_data() /* + bitmap->get_shm_offset() */,
							bitmap->get_y_offset(),
							bitmap->get_u_offset(),
							bitmap->get_v_offset());
			}
		}

// Create new bitmap
		if(!bitmap)
		{
// Try hardware accelerated
			switch(best_colormodel)
			{
				case BC_YUV420P:
					if(device->out_config->driver == PLAYBACK_X11_XV &&
						output->get_canvas()->accel_available(best_colormodel, 0) &&
						!output->use_scrollbars)
					{
						bitmap = new BC_Bitmap(output->get_canvas(), 
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
						output->get_canvas()->accel_available(best_colormodel, 0) &&
						!output->use_scrollbars)
					{
						bitmap = new BC_Bitmap(output->get_canvas(), 
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
						output->get_canvas()->accel_available(BC_YUV422, 0))
					{
						bitmap = new BC_Bitmap(output->get_canvas(), 
							device->out_w,
							device->out_h,
							BC_YUV422,
							1);
						bitmap_type = BITMAP_TEMP;
					}
					break;

				case BC_YUV422:
					if(device->out_config->driver == PLAYBACK_X11_XV &&
						output->get_canvas()->accel_available(best_colormodel, 0) &&
						!output->use_scrollbars)
					{
						bitmap = new BC_Bitmap(output->get_canvas(), 
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
						output->get_canvas()->accel_available(BC_YUV422P, 0))
					{
						bitmap = new BC_Bitmap(output->get_canvas(), 
							device->out_w,
							device->out_h,
							BC_YUV422P,
							1);
						bitmap_type = BITMAP_TEMP;
					}
					break;
			}

// Try default colormodel
			if(!bitmap)
			{
				best_colormodel = output->get_canvas()->get_color_model();
				bitmap = new BC_Bitmap(output->get_canvas(), 
					output->get_canvas()->get_w(),
					output->get_canvas()->get_h(),
					best_colormodel,
					1);
				bitmap_type = BITMAP_TEMP;
			}

			if(bitmap_type == BITMAP_TEMP)
			{
// Intermediate frame
				output_frame = new VFrame(0, 
					device->out_w,
					device->out_h,
					colormodel);
// printf("VDeviceX11::new_outout_buffer %p %d %d %d %p\n", 
// device,
// device->out_w,
// device->out_h,
// colormodel,
// output_frame->get_rows());
//BUFFER2(output_frame->get_rows()[0], "VDeviceX11::new_output_buffer 2");
				bitmap_type = BITMAP_TEMP;
			}
			color_model_selected = 1;
		}

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
	}


	*result = output_frame;

	output->get_canvas()->unlock_window();
	output->unlock_canvas();
//printf("VDeviceX11::new_output_buffer 10\n");
}


int VDeviceX11::start_playback()
{
// Record window is initialized when its monitor starts.
	if(!device->single_frame)
		output->start_video();
	return 0;
}

int VDeviceX11::stop_playback()
{
	if(!device->single_frame)
		output->stop_video();
// Record window goes back to monitoring
// get the last frame played and store it in the video_out
	return 0;
}

int VDeviceX11::write_buffer(VFrame *output_channels, EDL *edl)
{
// The reason for not drawing single frame is that it is _always_ drawn 
// when drawing draw_refresh in cwindowgui and vwindowgui
	if (device->single_frame) 
		return 0;

	int i = 0;
	output->lock_canvas("VDeviceX11::write_buffer");
	output->get_canvas()->lock_window("VDeviceX11::write_buffer 1");


//printf("VDeviceX11::write_buffer %d\n", output->get_canvas()->get_video_on());
	output->get_transfers(edl, 
		output_x1, 
		output_y1, 
		output_x2, 
		output_y2, 
		canvas_x1, 
		canvas_y1, 
		canvas_x2, 
		canvas_y2,
// Canvas may be a different size than the temporary bitmap for pure software
		(bitmap_type == BITMAP_TEMP && !bitmap->hardware_scaling()) ? bitmap->get_w() : -1,
		(bitmap_type == BITMAP_TEMP && !bitmap->hardware_scaling()) ? bitmap->get_h() : -1);


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
// 			out_h );
// fflush(stdout);

//printf("VDeviceX11::write_buffer 2\n");


		if(bitmap->hardware_scaling())
		{
			cmodel_transfer(bitmap->get_row_pointers(), 
				output_channels->get_rows(),
				0,
				0,
				0,
				output_channels->get_y(),
				output_channels->get_u(),
				output_channels->get_v(),
				0, 
				0, 
				output_channels->get_w(), 
				output_channels->get_h(),
				0, 
				0, 
				bitmap->get_w(), 
				bitmap->get_h(),
				output_channels->get_color_model(), 
				bitmap->get_color_model(),
				0,
				output_channels->get_w(),
				bitmap->get_w());
		}
		else
		{
			cmodel_transfer(bitmap->get_row_pointers(), 
				output_channels->get_rows(),
				0,
				0,
				0,
				output_channels->get_y(),
				output_channels->get_u(),
				output_channels->get_v(),
				(int)output_x1, 
				(int)output_y1, 
				(int)(output_x2 - output_x1), 
				(int)(output_y2 - output_y1),
				0, 
				0, 
				(int)(canvas_x2 - canvas_x1), 
				(int)(canvas_y2 - canvas_y1),
				output_channels->get_color_model(), 
				bitmap->get_color_model(),
				0,
				output_channels->get_w(),
				bitmap->get_w());
		}
	}

//printf("VDeviceX11::write_buffer 4 %p\n", bitmap);
//for(i = 0; i < 1000; i += 4) bitmap->get_data()[i] = 128;
//printf("VDeviceX11::write_buffer 2 %d %d %d\n", bitmap_type, 
//	bitmap->get_color_model(), 
//	output->get_color_model());fflush(stdout);
// printf("VDeviceX11::write_buffer 2 %d %d, %f %f %f %f -> %f %f %f %f\n",
// output->w,
// output->h,
// output_x1, 
// output_y1, 
// output_x2, 
// output_y2, 
// canvas_x1, 
// canvas_y1, 
// canvas_x2, 
// canvas_y2);



// Cause X server to display it
	if(device->out_config->driver == PLAYBACK_X11_GL)
	{
// Output is drawn in close_all if no video.
		if(output->get_canvas()->get_video_on())
		{
// Draw output frame directly.  Not used for compositing.
			output->get_canvas()->unlock_window();
			output->unlock_canvas();
			output->mwindow->playback_3d->write_buffer(output, 
				output_frame,
				output_x1,
				output_y1,
				output_x2,
				output_y2,
				canvas_x1,
				canvas_y1,
				canvas_x2,
				canvas_y2,
				is_cleared);
			is_cleared = 0;
			output->lock_canvas("VDeviceX11::write_buffer 2");
			output->get_canvas()->lock_window("VDeviceX11::write_buffer 2");
		}
	}
	else
	if(bitmap->hardware_scaling())
	{
		output->get_canvas()->draw_bitmap(bitmap,
			!device->single_frame,
			(int)canvas_x1,
			(int)canvas_y1,
			(int)(canvas_x2 - canvas_x1),
			(int)(canvas_y2 - canvas_y1),
			(int)output_x1,
			(int)output_y1,
			(int)(output_x2 - output_x1),
			(int)(output_y2 - output_y1),
			0);
	}
	else
	{
		output->get_canvas()->draw_bitmap(bitmap,
			!device->single_frame,
			(int)canvas_x1,
			(int)canvas_y1,
			(int)(canvas_x2 - canvas_x1),
			(int)(canvas_y2 - canvas_y1),
			0, 
			0, 
			(int)(canvas_x2 - canvas_x1),
			(int)(canvas_y2 - canvas_y1),
			0);
	}


	output->get_canvas()->unlock_window();
	output->unlock_canvas();
	return 0;
}


void VDeviceX11::clear_output()
{
	is_cleared = 1;

	output->mwindow->playback_3d->clear_output(output,
		output->get_canvas()->get_video_on() ? 0 : output_frame);

}


void VDeviceX11::clear_input(VFrame *frame)
{
	this->output->mwindow->playback_3d->clear_input(this->output, frame);
}

void VDeviceX11::do_camera(VFrame *output,
	VFrame *input,
	float in_x1, 
	float in_y1, 
	float in_x2, 
	float in_y2, 
	float out_x1, 
	float out_y1, 
	float out_x2, 
	float out_y2)
{
	this->output->mwindow->playback_3d->do_camera(this->output, 
		output,
		input,
		in_x1, 
		in_y1, 
		in_x2, 
		in_y2, 
		out_x1, 
		out_y1, 
		out_x2, 
		out_y2);
}


void VDeviceX11::do_fade(VFrame *output_temp, float fade)
{
	this->output->mwindow->playback_3d->do_fade(this->output, output_temp, fade);
}

void VDeviceX11::do_mask(VFrame *output_temp, 
		int64_t start_position_project,
		MaskAutos *keyframe_set, 
		MaskAuto *keyframe,
		MaskAuto *default_auto)
{
	this->output->mwindow->playback_3d->do_mask(output,
		output_temp,
		start_position_project,
		keyframe_set,
		keyframe,
		default_auto);
}

void VDeviceX11::overlay(VFrame *output_frame,
		VFrame *input, 
// This is the transfer from track to output frame
		float in_x1, 
		float in_y1, 
		float in_x2, 
		float in_y2, 
		float out_x1, 
		float out_y1, 
		float out_x2, 
		float out_y2, 
		float alpha,        // 0 - 1
		int mode,
		EDL *edl)
{
	int interpolation_type = edl->session->interpolation_type;

// printf("VDeviceX11::overlay 1:\n"
// "in_x1=%f in_y1=%f in_x2=%f in_y2=%f\n"
// "out_x1=%f out_y1=%f out_x2=%f out_y2=%f\n",
// in_x1, 
// in_y1, 
// in_x2, 
// in_y2, 
// out_x1,
// out_y1,
// out_x2,
// out_y2);
// Convert node coords to canvas coords in here
	output->lock_canvas("VDeviceX11::overlay");
	output->get_canvas()->lock_window("VDeviceX11::overlay");

// This is the transfer from output frame to canvas
	output->get_transfers(edl, 
		output_x1, 
		output_y1, 
		output_x2, 
		output_y2, 
		canvas_x1, 
		canvas_y1, 
		canvas_x2, 
		canvas_y2,
		-1,
		-1);

	output->get_canvas()->unlock_window();
	output->unlock_canvas();


// If single frame playback, use full sized PBuffer as output.
	if(device->single_frame)
	{
		output->mwindow->playback_3d->overlay(output, 
			input,
			in_x1, 
			in_y1, 
			in_x2, 
			in_y2, 
			out_x1,
			out_y1,
			out_x2,
			out_y2,
			alpha,  	  // 0 - 1
			mode,
			interpolation_type,
			output_frame);
// printf("VDeviceX11::overlay 1 %p %d %d %d\n", 
// output_frame, 
// output_frame->get_w(),
// output_frame->get_h(),
// output_frame->get_opengl_state());
	}
	else
	{

// Get transfer from track to canvas
		float track_xscale = (out_x2 - out_x1) / (in_x2 - in_x1);
		float track_yscale = (out_y2 - out_y1) / (in_y2 - in_y1);
		float canvas_xscale = (float)(canvas_x2 - canvas_x1) / (output_x2 - output_x1);
		float canvas_yscale = (float)(canvas_y2 - canvas_y1) / (output_y2 - output_y1);


// Get coordinates of canvas relative to track frame
		float track_x1 = (float)(output_x1 - out_x1) / track_xscale + in_x1;
		float track_y1 = (float)(output_y1 - out_y1) / track_yscale + in_y1;
		float track_x2 = (float)(output_x2 - out_x2) / track_xscale + in_x2;
		float track_y2 = (float)(output_y2 - out_y2) / track_yscale + in_y2;

// Clamp canvas coords to track boundary
		if(track_x1 < 0)
		{
			float difference = -track_x1;
			track_x1 += difference;
			canvas_x1 += difference * track_xscale * canvas_xscale;
		}
		if(track_y1 < 0)
		{
			float difference = -track_y1;
			track_y1 += difference;
			canvas_y1 += difference * track_yscale * canvas_yscale;
		}

		if(track_x2 > input->get_w())
		{
			float difference = track_x2 - input->get_w();
			track_x2 -= difference;
			canvas_x2 -= difference * track_xscale * canvas_xscale;
		}
		if(track_y2 > input->get_h())
		{
			float difference = track_y2 - input->get_h();
			track_y2 -= difference;
			canvas_y2 -= difference * track_yscale * canvas_yscale;
		}





// Overlay directly from track buffer to canvas, skipping output buffer
		if(track_x2 > track_x1 && 
			track_y2 > track_y1 &&
			canvas_x2 > canvas_x1 &&
			canvas_y2 > canvas_y1)
		{
			output->mwindow->playback_3d->overlay(output, 
				input,
				track_x1, 
				track_y1, 
				track_x2, 
				track_y2, 
				canvas_x1,
				canvas_y1,
				canvas_x2,
				canvas_y2,
				alpha,  	  // 0 - 1
				mode,
				interpolation_type);
		}
	}
}

void VDeviceX11::run_plugin(PluginClient *client)
{
	output->mwindow->playback_3d->run_plugin(output, client);
}

void VDeviceX11::copy_frame(VFrame *dst, VFrame *src)
{
	output->mwindow->playback_3d->copy_from(output, dst, src, 1);
}


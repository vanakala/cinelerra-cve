
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
#include "vdevicex11.h"
#include "vframe.h"
#include "videodevice.h"

#include <string.h>
#include <unistd.h>

VDeviceX11::VDeviceX11(VideoDevice *device, Canvas *output)
 : VDeviceBase(device)
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
	gl_failed = 0;
	num_xv_cmodels = -1;
	accel_cmodel = -1;
	this->output = output;
}

VDeviceX11::~VDeviceX11()
{
	if(output && output_frame)
	{
// Copy our output frame buffer to the canvas's permanent frame buffer.
// They must be different buffers because the output frame is being written
// while the user is redrawing the canvas frame buffer over and over.
		output->lock_canvas("VDeviceX11::~VDeviceX11");
		int use_opengl = device->out_config->driver == PLAYBACK_X11_GL &&
			!gl_failed &&
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
			output->mwindow->playback_3d->copy_from(output, 
				output->refresh_frame,
				output_frame,
				0);
		}
		else
			output->refresh_frame->copy_from(output_frame);

		if(!device->single_frame)
		{
			output->stop_video();
		}
		else
		{
			output->stop_single();
		}

// Draw the first refresh with new frame.
		output->draw_refresh();
		output->unlock_canvas();
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
}

int VDeviceX11::open_input()
{
	capture_bitmap = new BC_Capture(device->in_config->w, 
		device->in_config->h,
		device->in_config->screencapture_display);

	return 0;
}

int VDeviceX11::open_output()
{
	if(output)
	{
		if(!device->single_frame)
			output->start_video();
		else
			output->start_single();
	}
	return 0;
}

int VDeviceX11::output_visible()
{
	if(!output) return 0;

	return !output->get_canvas()->get_hidden();
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
	int c1, c2;

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
		if(device->out_config->driver == PLAYBACK_X11_XV)
		{
			if(accel_cmodel >= 0)
				return accel_cmodel;

			if(num_xv_cmodels < 0)
				num_xv_cmodels = output->get_canvas()->accel_cmodels(xv_cmodels, MAX_XV_CMODELS);

// Select the best of hw supported cmodels
// If we haven't exact cmodel, assume yuv is best to be converted
// to yuv, rgb to rgb
			c1 = c2 = -1;
			for(int i = 0; i < num_xv_cmodels; i++)
			{
				if(colormodel == xv_cmodels[i])
				{
					c1 = xv_cmodels[i];
					break;
				}
				if(c2 < 0)
				{
					if(cmodel_is_yuv(colormodel))
					{
						if(cmodel_is_yuv(xv_cmodels[i]))
							c2 = xv_cmodels[i];
					}
					else
					{
						if(!cmodel_is_yuv(xv_cmodels[i]))
							c2 = xv_cmodels[i];
					}
				}
			}
			accel_cmodel = result = c1 > 0 ? c1 : c2;
		}
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
			result = output->get_canvas()->get_color_model();
			break;
		}
	}
	return result;
}

void VDeviceX11::new_output_buffer(VFrame **result, int colormodel)
{
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
				output_frame->set_memory((unsigned char*)bitmap->get_data(),
							bitmap->get_y_offset(),
							bitmap->get_u_offset(),
							bitmap->get_v_offset());
			}
		}

// Create new bitmap
		if(!bitmap)
		{
// Try hardware accelerated
			if(device->out_config->driver == PLAYBACK_X11_XV &&
				output->get_canvas()->accel_available(best_colormodel, device->out_w, device->out_h))
			{
				bitmap = new BC_Bitmap(output->get_canvas(), 
					device->out_w,
					device->out_h,
					best_colormodel,
					1);
				if(!output->use_scrollbars && colormodel == best_colormodel)
				{
					output_frame = new VFrame((unsigned char*)bitmap->get_data(),
						bitmap->get_y_offset(),
						bitmap->get_u_offset(),
						bitmap->get_v_offset(),
						device->out_w,
						device->out_h,
						best_colormodel);
					bitmap_type = BITMAP_PRIMARY;
				}
				else
					bitmap_type = BITMAP_TEMP;
			}
			else
			{
// Try default colormodel
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
				bitmap_type = BITMAP_TEMP;
			}
			color_model_selected = 1;
		}
	}
	*result = output_frame;
}

int VDeviceX11::write_buffer(VFrame *output_channels, EDL *edl)
{
// The reason for not drawing single frame is that it is _always_ drawn 
// when drawing draw_refresh in cwindowgui and vwindowgui
	if (device->single_frame)
		return 0;

	int i = 0;
	output->lock_canvas("VDeviceX11::write_buffer");

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
		if(bitmap->hardware_scaling())
		{
			cmodel_transfer(bitmap->get_row_pointers(), 
				output_channels->get_rows(),
				bitmap->get_y_plane(),
				bitmap->get_u_plane(),
				bitmap->get_v_plane(),
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

// Cause X server to display it
	if(device->out_config->driver == PLAYBACK_X11_GL)
	{
// Output is drawn in close_all if no video.
		if(output->get_canvas()->get_video_on())
		{
// Draw output frame directly.  Not used for compositing.
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

	output->unlock_canvas();
	return 0;
}

int VDeviceX11::clear_output()
{
	is_cleared = 1;

	gl_failed = output->mwindow->playback_3d->clear_output(output,
		output->get_canvas()->get_video_on() ? 0 : output_frame);

	return gl_failed;
}

void VDeviceX11::clear_input(VFrame *frame)
{
	this->output->mwindow->playback_3d->clear_input(this->output, frame);
}

int VDeviceX11::do_camera(VFrame *output,
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
	gl_failed = this->output->mwindow->playback_3d->do_camera(this->output,
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
	return gl_failed;
}

void VDeviceX11::do_fade(VFrame *output_temp, float fade)
{
	this->output->mwindow->playback_3d->do_fade(this->output, output_temp, fade);
}

void VDeviceX11::do_mask(VFrame *output_temp, 
		MaskAutos *keyframe_set, 
		MaskAuto *keyframe)
{
	this->output->mwindow->playback_3d->do_mask(output,
		output_temp,
		output_temp->get_pts() / output_temp->get_duration(),
		keyframe_set,
		keyframe);
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

// Convert node coords to canvas coords in here
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
			alpha,           // 0 - 1
			mode,
			interpolation_type,
			output_frame);
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
				alpha,          // 0 - 1
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

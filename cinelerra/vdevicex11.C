
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
#include "bcbitmap.h"
#include "bccapture.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "canvas.h"
#include "colormodels.h"
#include "edl.h"
#include "edlsession.h"
#include "mwindow.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "tmpframecache.h"
#include "vdevicex11.h"
#include "vframe.h"
#include "videodevice.h"

#include <string.h>
#include <unistd.h>

VDeviceX11::VDeviceX11(VideoDevice *device, Canvas *output)
 : VDeviceBase(device)
{
	output_frame = 0;
	bitmap = 0;
	output_x1 = 0;
	output_y1 = 0;
	output_x2 = 0;
	output_y2 = 0;
	canvas_x1 = 0;
	canvas_y1 = 0;
	canvas_x2 = 0;
	canvas_y2 = 0;
	is_cleared = 0;
	num_xv_cmodels = -1;
	accel_cmodel = -1;
	this->output = output;
}

VDeviceX11::~VDeviceX11()
{
	if(output && output_frame)
	{
// Use our output frame buffer as the canvas's frame buffer.
		output->lock_canvas("VDeviceX11::~VDeviceX11");

		if(!device->single_frame)
			output->stop_video();
		else
			output->stop_single();

		if(output_frame)
		{
			BC_Resources::tmpframes.release_frame(output->refresh_frame);
			output->refresh_frame = output_frame;
		}

// Draw the first refresh with new frame.
		output->draw_refresh();
		output->unlock_canvas();
	}

	delete bitmap;
}

int VDeviceX11::open_output()
{
	if(output)
	{
		if(device->out_config->driver == PLAYBACK_X11_GL)
		{
			if(output->get_canvas()->enable_opengl())
				device->out_config->driver = PLAYBACK_X11_XV;
		}
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
					if(ColorModels::is_yuv(colormodel))
					{
						if(ColorModels::is_yuv(xv_cmodels[i]))
							c2 = xv_cmodels[i];
					}
					else
					{
						if(!ColorModels::is_yuv(xv_cmodels[i]))
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

VFrame *VDeviceX11::new_output_buffer(int colormodel)
{
// Get the best colormodel the display can handle.
	int best_colormodel = get_best_colormodel(colormodel);

// Create new bitmap
	if(!bitmap)
	{
// Try hardware accelerated
		if(device->out_config->driver == PLAYBACK_X11_XV &&
			output->get_canvas()->accel_available(best_colormodel, device->out_w, device->out_h))
		{
			bitmap = new BC_Bitmap(output->get_canvas(),
				0,
				0,
				best_colormodel,
				1);
		}
		if(!output_frame)
		{
// Intermediate frame
			output_frame = BC_Resources::tmpframes.get_tmpframe(
				device->out_w,
				device->out_h,
				colormodel);
		}
	}
	return output_frame;
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
		canvas_y2);
// Convert colormodel
	if(bitmap)
	{
		output_channels->set_pixel_aspect(1);
		bitmap->read_frame(output_channels,
			0, 0,
			output_channels->get_w(),
			output_channels->get_h());

		output->get_canvas()->draw_bitmap(bitmap,
			!device->single_frame,
			round(canvas_x1),
			round(canvas_y1),
			round(canvas_x2 - canvas_x1),
			round(canvas_y2 - canvas_y1),
			round(output_x1),
			round(output_y1),
			round(output_x2 - output_x1),
			round(output_y2 - output_y1),
			0);
	}
	else
	{
		output_channels->set_pixel_aspect(edl->get_sample_aspect_ratio());
		output->get_canvas()->draw_vframe(output_channels,
			round(canvas_x1),
			round(canvas_y1),
			round(canvas_x2 - canvas_x1),
			round(canvas_y2 - canvas_y1),
			round(output_x1),
			round(output_y1),
			round(output_x2 - output_x1),
			round(output_y2 - output_y1),
			0);
	}
	output->unlock_canvas();
	return 0;
}


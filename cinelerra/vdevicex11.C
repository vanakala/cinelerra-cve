// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcbitmap.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "canvas.h"
#include "colormodels.h"
#include "playbackconfig.h"
#include "tmpframecache.h"
#include "vdevicex11.h"
#include "vframe.h"
#include "videodevice.h"

#include <string.h>
#include <unistd.h>

VDeviceX11::VDeviceX11(VideoDevice *device, Canvas *output)
{
	bitmap = 0;
	output_x1 = 0;
	output_y1 = 0;
	output_x2 = 0;
	output_y2 = 0;
	canvas_x1 = 0;
	canvas_y1 = 0;
	canvas_x2 = 0;
	canvas_y2 = 0;
	num_xv_cmodels = -1;
	accel_cmodel = -1;
	this->output = output;
	this->device = device;
}

VDeviceX11::~VDeviceX11()
{
	if(output)
	{
// Use our output frame buffer as the canvas's frame buffer.
		output->lock_canvas("VDeviceX11::~VDeviceX11");

		if(!device->single_frame)
			output->stop_video();
		else
			output->stop_single();

// Draw the first refresh with new frame.
		output->draw_refresh();
		output->unlock_canvas();
	}

	delete bitmap;
}

int VDeviceX11::open_output(int colormodel)
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

		if(device->out_config->driver == PLAYBACK_X11_XV  && !bitmap)
		{
// Try hardware accelerated
			int accel_colormodel = get_accel_colormodel(colormodel);

			if(accel_colormodel > BC_NOCOLOR &&
				output->get_canvas()->accel_available(accel_colormodel,
					device->out_w, device->out_h))
			{
				bitmap = new BC_Bitmap(output->get_canvas(),
					0, 0, accel_colormodel, 1);
			}
		}
	}
	return 0;
}

int VDeviceX11::output_visible()
{
	if(!output) return 0;

	return !output->get_canvas()->get_hidden();
}

int VDeviceX11::get_accel_colormodel(int colormodel)
{
	int c1, c2;

	if(!device->single_frame)
	{
		if(device->out_config->driver == PLAYBACK_X11_XV)
		{
			if(accel_cmodel >= BC_NOCOLOR)
				return accel_cmodel;

			if(num_xv_cmodels < 0)
				num_xv_cmodels = output->get_canvas()->accel_cmodels(xv_cmodels, MAX_XV_CMODELS);

// Select the best of hw supported cmodels
// If we haven't exact cmodel, assume yuv is best to be converted
// to yuv, rgb to rgb
			c1 = c2 = BC_NOCOLOR;
			for(int i = 0; i < num_xv_cmodels; i++)
			{
				if(colormodel == xv_cmodels[i])
				{
					c1 = xv_cmodels[i];
					break;
				}
				if(c2 == BC_NOCOLOR)
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
			accel_cmodel = c1 != BC_NOCOLOR ? c1 : c2;
		}
	}
	return accel_cmodel;
}

int VDeviceX11::write_buffer(VFrame *output_channels)
{
	if(output_channels != output->refresh_frame)
	{
		BC_Resources::tmpframes.release_frame(output->refresh_frame);
		output->refresh_frame = output_channels;
	}
// The reason for not drawing single frame is that it is _always_ drawn 
// when drawing draw_refresh in cwindowgui and vwindowgui
	if (device->single_frame)
		return 0;

	int i = 0;
	output->lock_canvas("VDeviceX11::write_buffer");

	output->get_transfers(output_x1,
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


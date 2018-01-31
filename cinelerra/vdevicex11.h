
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

#ifndef VDEVICEX11_H
#define VDEVICEX11_H

#define MAX_XV_CMODELS 16

#include "canvas.inc"
#include "edl.inc"
#include "maskauto.inc"
#include "maskautos.inc"
#include "pluginclient.inc"
#include "thread.h"
#include "vdevicebase.h"


class VDeviceX11 : public VDeviceBase
{
public:
	VDeviceX11(VideoDevice *device, Canvas *output);
	~VDeviceX11();

// User always gets the colormodel requested
	void new_output_buffer(VFrame **output, int colormodel);

	int open_output();
	int output_visible();

// After loading the bitmap with a picture, write it
	int write_buffer(VFrame *result, EDL *edl);

private:

// Closest colormodel the hardware can do for playback.
// Only used by VDeviceX11::new_output_buffer.  The value from File::get_best_colormodel
// is passed to this to create the VFrame to which the output is rendered.
// For OpenGL, it creates the array of row pointers used to upload the video
// frame to the texture, the texture, and the PBuffer.
	int get_best_colormodel(int colormodel);

// Bitmap to be written to device
	BC_Bitmap *bitmap;
// Wrapper for bitmap or intermediate buffer for user to write to
	VFrame *output_frame;
// Canvas for output
	Canvas *output;
	int color_model;
// Transfer coordinates from the output frame to the canvas 
// for last frame rendered.
// These stick the last frame to the display.
// Must be floats to support OpenGL
	double output_x1, output_y1, output_x2, output_y2;
	double canvas_x1, canvas_y1, canvas_x2, canvas_y2;
// Set when OpenGL rendering has cleared the frame buffer before write_buffer
	int is_cleared;
// XV accelerated colormodels
	int accel_cmodel;
	int num_xv_cmodels;
	int xv_cmodels[MAX_XV_CMODELS];
};

#endif

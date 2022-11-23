// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef VDEVICEX11_H
#define VDEVICEX11_H

#define MAX_XV_CMODELS 16

#include "bcbitmap.inc"
#include "canvas.inc"
#include "videodevice.inc"
#include "vframe.inc"

class VDeviceX11
{
public:
	VDeviceX11(VideoDevice *device, Canvas *output);
	~VDeviceX11();

	int open_output(int colormodel);
	int output_visible();

// After loading the bitmap with a picture, write it
	int write_buffer(VFrame *result);

private:

// Closest colormodel the hardware can do for playback.
// Only used by VDeviceX11::new_output_buffer.
	int get_accel_colormodel(int colormodel);

// Bitmap to be written to device
	BC_Bitmap *bitmap;

// Canvas for output
	Canvas *output;
	VideoDevice *device;
// Transfer coordinates from the output frame to the canvas 
// for last frame rendered.
// These stick the last frame to the display.
// Must be floats to support OpenGL
	double output_x1, output_y1, output_x2, output_y2;
	double canvas_x1, canvas_y1, canvas_x2, canvas_y2;
// XV accelerated colormodels
	int accel_cmodel;
	int num_xv_cmodels;
	int xv_cmodels[MAX_XV_CMODELS];
};

#endif

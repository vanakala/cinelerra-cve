
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

#ifndef BCCAPTURE_H
#define BCCAPTURE_H

#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>

#include "sizes.h"
#include "vframe.inc"


class BC_Capture
{
public:
	BC_Capture(int w, int h, char *display_path = "");
	virtual ~BC_Capture();

	int init_window(char *display_path);
// x1 and y1 are automatically adjusted if out of bounds
	int capture_frame(VFrame *frame, int &x1, int &y1);
	int get_w();
	int get_h();

	int w, h, default_depth;
	unsigned char **row_data;

private:
	int allocate_data();
	int delete_data();
	int get_top_w();
	int get_top_h();
	
	inline void import_RGB565_to_RGB888(unsigned char* &output, unsigned char* &input)
	{
		*output++ = (*input & 0xf800) >> 8;
		*output++ = (*input & 0x7e0) >> 3;
		*output++ = (*input & 0x1f) << 3;

		input += 2;
		output += 3;
	};

	inline void import_BGR888_to_RGB888(unsigned char* &output, unsigned char* &input)
	{
		*output++ = input[2];
		*output++ = input[1];
		*output++ = input[0];

		input += 3;
		output += 3;
	};

	inline void import_BGR8888_to_RGB888(unsigned char* &output, unsigned char* &input)
	{
		*output++ = input[2];
		*output++ = input[1];
		*output++ = input[0];

		input += 4;
		output += 3;
	};

	int use_shm;
	int bitmap_color_model;
	unsigned char *data;
	XImage *ximage;
	XShmSegmentInfo shm_info;
	Display* display;
	Window rootwin;
	Visual *vis;
	int bits_per_pixel;
	int screen;
	long shm_event_type;
	int client_byte_order, server_byte_order;
};

#endif

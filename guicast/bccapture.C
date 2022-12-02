// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bccapture.h"
#include "bcsignals.h"
#include "bcwindowbase.h"
#include "colormodels.h"
#include "language.h"
#include "vframe.h"

#include <stdlib.h>
#include <stdio.h>
#include <X11/Xutil.h>

// Byte orders:
// 24 bpp packed:         bgr
// 24 bpp unpacked:       0bgr

BC_Capture::BC_Capture(int w, int h, char *display_path)
{
	this->w = w;
	this->h = h;

	data = 0;
	use_shm = 1;
	init_window(display_path);
	allocate_data();
}

BC_Capture::~BC_Capture()
{
	delete_data();
	XCloseDisplay(display);
}

void BC_Capture::init_window(char *display_path)
{
	int bits_per_pixel;

	if(display_path && display_path[0] == 0)
		display_path = NULL;

	if((display = XOpenDisplay(display_path)) == NULL)
	{
		printf(_("cannot connect to X server.\n"));
		if(getenv("DISPLAY") == NULL)
			printf(_("'DISPLAY' environment variable not set.\n"));
		exit(-1);
	}

	screen = DefaultScreen(display);
	rootwin = RootWindow(display, screen);
	vis = DefaultVisual(display, screen);
	default_depth = DefaultDepth(display, screen);
	client_byte_order = (*(const u_int32_t*)"a   ") & 0x00000001;
	server_byte_order = (XImageByteOrder(display) == MSBFirst) ? 0 : 1;
	char *data = 0;
	XImage *ximage;
	ximage = XCreateImage(display, vis, default_depth,
		ZPixmap, 0, data, 16, 16, 8, 0);
	bits_per_pixel = ximage->bits_per_pixel;
	XDestroyImage(ximage);

	bitmap_color_model = BC_WindowBase::evaluate_color_model(client_byte_order,
		server_byte_order, bits_per_pixel);

// test shared memory
// This doesn't ensure the X Server is on the local host
	if(use_shm && !XShmQueryExtension(display))
		use_shm = 0;
}

void BC_Capture::allocate_data()
{
// try shared memory
	if(use_shm)
	{
		ximage = XShmCreateImage(display, vis, default_depth,
			ZPixmap, (char*)NULL, &shm_info, w, h);

		shm_info.shmid = shmget(IPC_PRIVATE, h * ximage->bytes_per_line,
			IPC_CREAT | 0600);

		if(shm_info.shmid == -1)
		{
			perror("BC_Capture::allocate_data shmget");
			abort();
		}

		data = (unsigned char *)shmat(shm_info.shmid, NULL, 0);
		shmctl(shm_info.shmid, IPC_RMID, 0);
		ximage->data = shm_info.shmaddr = (char*)data;  // setting ximage->data stops BadValue
		shm_info.readOnly = 0;

// Crashes here if remote server.
		BC_Signals::set_catch_errors();
		XShmAttach(display, &shm_info);
		XSync(display, False);

		if(BC_Signals::reset_catch())
		{
			XDestroyImage(ximage);
			shmdt(shm_info.shmaddr);
			use_shm = 0;
		}
	}

	if(!use_shm)
	{
// need to use bytes_per_line for some X servers
		data = 0;
		ximage = XCreateImage(display, vis, default_depth,
			ZPixmap, 0, (char*)data, w, h, 8, 0);
		data = (unsigned char*)malloc(h * ximage->bytes_per_line);
		XDestroyImage(ximage);

		ximage = XCreateImage(display, vis, default_depth, ZPixmap, 0, (char*)data, w, h, 8, 0);
	}

// This differs from the depth parameter of the top_level.
	bits_per_pixel = ximage->bits_per_pixel;
	bytes_per_line = ximage->bytes_per_line;
}

void BC_Capture::delete_data()
{
	if(data)
	{
		if(use_shm)
		{
			XShmDetach(display, &shm_info);
			XDestroyImage(ximage);
			shmdt(shm_info.shmaddr);
		}
		else
			XDestroyImage(ximage);

// data is automatically freed by XDestroyImage
		data = 0;
	}
}

int BC_Capture::capture_frame(VFrame *frame, int &x1, int &y1)
{
	int top_dim;

	if(!display)
		return 1;

	if(x1 < 0)
		x1 = 0;
	if(y1 < 0)
		y1 = 0;

	top_dim = get_top_w();
	if(x1 > top_dim - w)
		x1 = top_dim - w;
	top_dim = get_top_h();
	if(y1 > top_dim - h)
		y1 = top_dim - h;

// Read the raw data
	if(use_shm)
		XShmGetImage(display, rootwin, ximage, x1, y1, 0xffffffff);
	else
		XGetSubImage(display, rootwin, x1, y1, w, h, 0xffffffff, ZPixmap, ximage, 0, 0);

	ColorModels::transfer_sws(frame->get_data(), data,
		frame->get_y(), frame->get_u(), frame->get_v(),
		0, 0, 0, w, h,
		frame->get_w(), frame->get_h(),
		bitmap_color_model, frame->get_color_model(),
		bytes_per_line, frame->get_bytes_per_line());
	return 0;
}

int BC_Capture::get_top_w()
{
	Screen *screen_ptr = XDefaultScreenOfDisplay(display);
	return WidthOfScreen(screen_ptr);
}

int BC_Capture::get_top_h()
{
	Screen *screen_ptr = XDefaultScreenOfDisplay(display);
	return HeightOfScreen(screen_ptr);
}

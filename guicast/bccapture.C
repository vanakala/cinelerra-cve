
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

#include "bccapture.h"
#include "bcresources.h"
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

int BC_Capture::init_window(char *display_path)
{
	int bits_per_pixel;
	if(display_path && display_path[0] == 0) display_path = NULL;
	if((display = XOpenDisplay(display_path)) == NULL)
	{
  		printf(_("cannot connect to X server.\n"));
  		if(getenv("DISPLAY") == NULL)
    		printf(_("'DISPLAY' environment variable not set.\n"));
  		exit(-1);
		return 1;
 	}

	screen = DefaultScreen(display);
	rootwin = RootWindow(display, screen);
	vis = DefaultVisual(display, screen);
	default_depth = DefaultDepth(display, screen);
	client_byte_order = (*(u_int32_t*)"a   ") & 0x00000001;
	server_byte_order = (XImageByteOrder(display) == MSBFirst) ? 0 : 1;
	char *data = 0;
	XImage *ximage;
	ximage = XCreateImage(display, 
					vis, 
					default_depth, 
					ZPixmap, 
					0, 
					data, 
					16, 
					16, 
					8, 
					0);
	bits_per_pixel = ximage->bits_per_pixel;
	XDestroyImage(ximage);
	bitmap_color_model = BC_WindowBase::evaluate_color_model(client_byte_order, server_byte_order, bits_per_pixel);

// test shared memory
// This doesn't ensure the X Server is on the local host
    if(use_shm && !XShmQueryExtension(display))
    {
        use_shm = 0;
    }
	return 0;
}


int BC_Capture::allocate_data()
{
// try shared memory
	if(!display) return 1;
    if(use_shm)
	{
	    ximage = XShmCreateImage(display, vis, default_depth, ZPixmap, (char*)NULL, &shm_info, w, h);

		shm_info.shmid = shmget(IPC_PRIVATE, h * ximage->bytes_per_line, IPC_CREAT | 0777);
		if(shm_info.shmid < 0) perror("BC_Capture::allocate_data shmget");
		data = (unsigned char *)shmat(shm_info.shmid, NULL, 0);
		shmctl(shm_info.shmid, IPC_RMID, 0);
		ximage->data = shm_info.shmaddr = (char*)data;  // setting ximage->data stops BadValue
		shm_info.readOnly = 0;

// Crashes here if remote server.
		BC_Resources::error = 0;
		XShmAttach(display, &shm_info);
    	XSync(display, False);
		if(BC_Resources::error)
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
		ximage = XCreateImage(display, vis, default_depth, ZPixmap, 0, (char*)data, w, h, 8, 0);
		data = (unsigned char*)malloc(h * ximage->bytes_per_line);
		XDestroyImage(ximage);

		ximage = XCreateImage(display, vis, default_depth, ZPixmap, 0, (char*)data, w, h, 8, 0);
	}

	row_data = new unsigned char*[h];
	for(int i = 0; i < h; i++)
	{
		row_data[i] = &data[i * ximage->bytes_per_line];
	}
// This differs from the depth parameter of the top_level.
	bits_per_pixel = ximage->bits_per_pixel;
	return 0;
}

int BC_Capture::delete_data()
{
	if(!display) return 1;
	if(data)
	{
		if(use_shm)
		{
			XShmDetach(display, &shm_info);
			XDestroyImage(ximage);
			shmdt(shm_info.shmaddr);
		}
		else
		{
			XDestroyImage(ximage);
		}

// data is automatically freed by XDestroyImage
		data = 0;
		delete row_data;
	}
	return 0;
}


int BC_Capture::get_w() { return w; }
int BC_Capture::get_h() { return h; }

// Capture a frame from the screen
#define CAPTURE_FRAME_HEAD \
	for(int i = 0; i < h; i++) \
	{ \
		unsigned char *input_row = row_data[i]; \
		unsigned char *output_row = (unsigned char*)frame->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{

#define CAPTURE_FRAME_TAIL \
		} \
	}



int BC_Capture::capture_frame(VFrame *frame, int &x1, int &y1)
{
	if(!display) return 1;
	if(x1 < 0) x1 = 0;
	if(y1 < 0) y1 = 0;
	if(x1 > get_top_w() - w) x1 = get_top_w() - w;
	if(y1 > get_top_h() - h) y1 = get_top_h() - h;


// Read the raw data
	if(use_shm)
		XShmGetImage(display, rootwin, ximage, x1, y1, 0xffffffff);
	else
		XGetSubImage(display, rootwin, x1, y1, w, h, 0xffffffff, ZPixmap, ximage, 0, 0);

	cmodel_transfer(frame->get_rows(), 
		row_data,
		frame->get_y(),
		frame->get_u(),
		frame->get_v(),
		0,
		0,
		0,
		0, 
		0, 
		w, 
		h,
		0, 
		0, 
		frame->get_w(), 
		frame->get_h(),
		bitmap_color_model, 
		frame->get_color_model(),
		0,
		frame->get_w(),
		w);

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

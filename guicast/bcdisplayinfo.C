
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

#include "bcdisplayinfo.h"
#include "clip.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>

#define TEST_SIZE 128
#define TEST_SIZE2 164
#define TEST_SIZE3 196
int BC_DisplayInfo::top_border = -1;
int BC_DisplayInfo::left_border = -1;
int BC_DisplayInfo::bottom_border = -1;
int BC_DisplayInfo::right_border = -1;
int BC_DisplayInfo::auto_reposition_x = -1;
int BC_DisplayInfo::auto_reposition_y = -1;


BC_DisplayInfo::BC_DisplayInfo(char *display_name, int show_error)
{
	init_window(display_name, show_error);
}

BC_DisplayInfo::~BC_DisplayInfo()
{
	XCloseDisplay(display);
}


void BC_DisplayInfo::parse_geometry(char *geom, int *x, int *y, int *width, int *height)
{
	XParseGeometry(geom, x, y, (unsigned int*)width, (unsigned int*)height);
}

void BC_DisplayInfo::test_window(int &x_out, 
	int &y_out, 
	int &x_out2, 
	int &y_out2, 
	int x_in, 
	int y_in)
{
	unsigned long mask = CWEventMask | CWWinGravity;
	XSetWindowAttributes attr;
	XSizeHints size_hints;

//printf("BC_DisplayInfo::test_window 1\n");
	x_out = 0;
	y_out = 0;
	x_out2 = 0;
	y_out2 = 0;
	attr.event_mask = StructureNotifyMask;
	attr.win_gravity = SouthEastGravity;
	Window win = XCreateWindow(display, 
			rootwin, 
			x_in, 
			y_in, 
			TEST_SIZE, 
			TEST_SIZE, 
			0, 
			default_depth, 
			InputOutput, 
			vis, 
			mask, 
			&attr);
	XGetNormalHints(display, win, &size_hints);
	size_hints.flags = PPosition | PSize;
	size_hints.x = x_in;
	size_hints.y = y_in;
	size_hints.width = TEST_SIZE;
	size_hints.height = TEST_SIZE;
	XSetStandardProperties(display, 
		win, 
		"x", 
		"x", 
		None, 
		0, 
		0, 
		&size_hints);

	XMapWindow(display, win); 
	XFlush(display);
	XSync(display, 0);
	XMoveResizeWindow(display, 
		win, 
		x_in, 
		y_in,
		TEST_SIZE2,
		TEST_SIZE2);
	XFlush(display);
	XSync(display, 0);

	XResizeWindow(display, 
		win, 
		TEST_SIZE3,
		TEST_SIZE3);
	XFlush(display);
	XSync(display, 0);

	XEvent event;
	int last_w = 0;
	int last_h = 0;
	int state = 0;

	do
	{
		XNextEvent(display, &event);
//printf("BC_DisplayInfo::test_window 1 event=%d %d\n", event.type, XPending(display));
		if(event.type == ConfigureNotify && event.xany.window == win)
		{
// Get creation repositioning
			if(last_w != event.xconfigure.width || last_h != event.xconfigure.height)
			{
				state++;
				last_w = event.xconfigure.width;
				last_h = event.xconfigure.height;
			}

			if(state == 1)
			{
				x_out = MAX(event.xconfigure.x + event.xconfigure.border_width - x_in, x_out);
				y_out = MAX(event.xconfigure.y + event.xconfigure.border_width - y_in, y_out);
			}
			else
			if(state == 2)
// Get moveresize repositioning
			{
				x_out2 = MAX(event.xconfigure.x + event.xconfigure.border_width - x_in, x_out2);
				y_out2 = MAX(event.xconfigure.y + event.xconfigure.border_width - y_in, y_out2);
			}
// printf("BC_DisplayInfo::test_window 2 state=%d x_out=%d y_out=%d x_in=%d y_in=%d w=%d h=%d\n",
// state,
// event.xconfigure.x + event.xconfigure.border_width, 
// event.xconfigure.y + event.xconfigure.border_width, 
// x_in, 
// y_in, 
// event.xconfigure.width, 
// event.xconfigure.height);
		}
 	}while(state != 3);

	XDestroyWindow(display, win);
	XFlush(display);
	XSync(display, 0);

	x_out = MAX(0, x_out);
	y_out = MAX(0, y_out);
	x_out = MIN(x_out, 30);
	y_out = MIN(y_out, 30);
//printf("BC_DisplayInfo::test_window 2\n");
}

void BC_DisplayInfo::init_borders()
{
	if(top_border < 0)
	{

		test_window(left_border, 
			top_border, 
			auto_reposition_x, 
			auto_reposition_y, 
			0, 
			0);
		right_border = left_border;
		bottom_border = left_border;
// printf("BC_DisplayInfo::init_borders border=%d %d auto=%d %d\n", 
// left_border, 
// top_border, 
// auto_reposition_x, 
// auto_reposition_y);
	}
}


int BC_DisplayInfo::get_top_border()
{
	init_borders();
	return top_border;
}

int BC_DisplayInfo::get_left_border()
{
	init_borders();
	return left_border;
}

int BC_DisplayInfo::get_right_border()
{
	init_borders();
	return right_border;
}

int BC_DisplayInfo::get_bottom_border()
{
	init_borders();
	return bottom_border;
}

void BC_DisplayInfo::init_window(char *display_name, int show_error)
{
	if(display_name && display_name[0] == 0) display_name = NULL;
	
// This function must be the first Xlib
// function a multi-threaded program calls
	XInitThreads();

	if((display = XOpenDisplay(display_name)) == NULL)
	{
		if(show_error)
		{
  			printf("BC_DisplayInfo::init_window: cannot connect to X server.\n");
  			if(getenv("DISPLAY") == NULL)
    			printf("'DISPLAY' environment variable not set.\n");
			exit(1);
		}
		return;
 	}
	
	screen = DefaultScreen(display);
	rootwin = RootWindow(display, screen);
	vis = DefaultVisual(display, screen);
	default_depth = DefaultDepth(display, screen);
}


int BC_DisplayInfo::get_root_w()
{
	Screen *screen_ptr = XDefaultScreenOfDisplay(display);
	return WidthOfScreen(screen_ptr);
}

int BC_DisplayInfo::get_root_h()
{
	Screen *screen_ptr = XDefaultScreenOfDisplay(display);
	return HeightOfScreen(screen_ptr);
}

int BC_DisplayInfo::get_abs_cursor_x()
{
	int abs_x, abs_y, win_x, win_y;
	unsigned int temp_mask;
	Window temp_win;

	XQueryPointer(display, 
	   rootwin, 
	   &temp_win, 
	   &temp_win,
       &abs_x, 
	   &abs_y, 
	   &win_x, 
	   &win_y, 
	   &temp_mask);
	return abs_x;
}

int BC_DisplayInfo::get_abs_cursor_y()
{
	int abs_x, abs_y, win_x, win_y;
	unsigned int temp_mask;
	Window temp_win;

	XQueryPointer(display, 
	   rootwin, 
	   &temp_win, 
	   &temp_win,
       &abs_x, 
	   &abs_y, 
	   &win_x, 
	   &win_y, 
	   &temp_mask);
	return abs_y;
}

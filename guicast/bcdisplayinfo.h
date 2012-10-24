
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

#ifndef BCDISPLAYINFO_H
#define BCDISPLAYINFO_H

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>

// XV info

#define XV_MAX_CMODELS 10

// Attributes we care about
#define NUM_XV_ATTRIBUTES 4
#define XV_ATTRIBUTE_BRIGHTNESS 0
#define XV_ATTRIBUTE_CONTRAST   1
#define XV_ATTRIBUTE_AUTOPAINT_COLORKEY 2
#define XV_ATTRIBUTE_COLORKEY  3

struct xv_attribute
{
	const char *name;
	int flags;
	int min;
	int max;
	int value;
	Atom xatom;
};

struct xv_adapterinfo
{
	char *adapter_name;
	int type;
	int base_port;
	int num_ports;
	int num_cmodels;
	int width;
	int height;
	struct xv_attribute attributes[NUM_XV_ATTRIBUTES];
	int cmodels[XV_MAX_CMODELS];
};


class BC_DisplayInfo
{
public:
	BC_DisplayInfo(const char *display_name = "", int show_error = 1);
	~BC_DisplayInfo();

	friend class BC_WindowBase;

	int get_root_w();
	int get_root_h();
	int get_abs_cursor_x();
	int get_abs_cursor_y();
	static void parse_geometry(char *geom, int *x, int *y, int *width, int *height);
// Get window border size created by window manager
	int get_top_border();
	int get_left_border();
	int get_right_border();
	int get_bottom_border();
	void test_window(int &x_out, int &y_out, int &x_out2, int &y_out2, int x_in, int y_in);
	int test_xvext();
	void dump_xvext();

// XV info
	static struct xv_adapterinfo *xv_adapters;
	static int num_adapters;
	unsigned int xv_version;
	unsigned int xv_release;
	unsigned int xv_req_base;
	unsigned int xv_event_base;
	unsigned int xv_error_base;

private:
	void init_borders();
	void init_window(const char *display_name, int show_error);
	Display* display;
	Window rootwin;
	Visual *vis;
	int screen;
	static int top_border;
	static int left_border;
	static int bottom_border;
	static int right_border;
	static int auto_reposition_x;
	static int auto_reposition_y;
	int default_depth;
	char *display_name;
};
#endif

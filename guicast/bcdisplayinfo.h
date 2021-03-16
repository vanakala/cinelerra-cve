// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BCDISPLAYINFO_H
#define BCDISPLAYINFO_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <X11/Xlib.h>

#include "bcdisplayinfo.inc"

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

	void get_root_size(int *width, int *height);
	void get_abs_cursor(int *abs_x, int *abs_y);
	static void parse_geometry(char *geom, int *x, int *y, int *width, int *height);
// Get window border size created by window manager
	int get_top_border();
	int get_left_border();
	int get_right_border();
	int get_bottom_border();
	void test_window(int &x_out, int &y_out, int &x_out2, int &y_out2);
	int test_xvext();
	static uint32_t cmodel_to_fourcc(int cmodel);
	void dump_xvext();
	static void dump_event(XEvent *ev);
	static const char *event_mode_str(int mode);
	static const char *event_detail_str(int detail);
	static const char *event_request_str(int req);
	static void print_event_common(XEvent *ev, const char *name);
	static const char *atom_name_str(Display *dpy, Atom atom);

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

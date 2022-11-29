// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcbitmap.h"
#include "bcpixmap.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcwindowbase.h"
#include "vframe.h"

#include <unistd.h>

BC_Pixmap::BC_Pixmap(BC_WindowBase *parent_window, 
	VFrame *frame, 
	int mode,
	int icon_offset)
{
	BC_Bitmap *opaque_bitmap, *alpha_bitmap, *mask_bitmap;

	reset();

	if(frame->get_color_model() != BC_RGBA8888 &&
			mode == PIXMAP_ALPHA)
		mode = PIXMAP_OPAQUE;
	this->mode = mode;

// Temporary bitmaps
	opaque_bitmap = new BC_Bitmap(parent_window,
		frame->get_w(), frame->get_h(),
		parent_window->get_color_model(), 0);
	opaque_bitmap->read_frame(frame, 0, 0,
		frame->get_w(), frame->get_h());

	if(mode == PIXMAP_ALPHA)
	{
		alpha_bitmap = new BC_Bitmap(parent_window,
				frame->get_w(), frame->get_h(),
				BC_TRANSPARENCY, 0);

		if(frame->get_color_model() != BC_RGBA8888)
			printf("BC_Pixmap::BC_Pixmap: PIXMAP_ALPHA but frame doesn't have alpha.\n");
		alpha_bitmap->read_frame(frame, 0, 0,
			frame->get_w(), frame->get_h());
	}

	initialize(parent_window, frame->get_w(), frame->get_h(), mode);

	opaque_bitmap->write_drawable(opaque_pixmap,
		top_level->gc, 0, 0, 0, 0, w, h);
	delete opaque_bitmap;

	if(mode == PIXMAP_ALPHA)
	{
		alpha_bitmap->write_drawable(alpha_pixmap, copy_gc, 0, 0,
			icon_offset ? 2 : 0, icon_offset ? 2 : 0, w, h);
		delete alpha_bitmap;
		XFreeGC(top_level->display, copy_gc);

		XSetClipMask(top_level->display, alpha_gc, alpha_pixmap);
	}
}

BC_Pixmap::BC_Pixmap(BC_WindowBase *parent_window, int w, int h)
{
	reset();
	initialize(parent_window, w, h, PIXMAP_OPAQUE);
}

BC_Pixmap::~BC_Pixmap()
{
	top_level->lock_window("BC_Pixmap::~BC_Pixmap");

	if(opaque_xft_draw)
		XftDrawDestroy((XftDraw*)opaque_xft_draw);
	XFreePixmap(top_level->display, opaque_pixmap);

	if(mode == PIXMAP_ALPHA)
	{
		XFreeGC(top_level->display, alpha_gc);
		if(alpha_xft_draw)
			XftDrawDestroy((XftDraw*)alpha_xft_draw);
		XFreePixmap(top_level->display, alpha_pixmap);
	}
	top_level->unlock_window();
}

void BC_Pixmap::reset()
{
	parent_window = 0;
	top_level = 0;
	opaque_pixmap = 0;
	alpha_pixmap = 0;
	opaque_xft_draw = 0;
	alpha_xft_draw = 0;
}

void BC_Pixmap::initialize(BC_WindowBase *parent_window, int w, int h, int mode)
{
	this->w = w;
	this->h = h;
	this->parent_window = parent_window;
	this->mode = mode;
	top_level = parent_window->top_level;

	top_level->lock_window("BC_Pixmap::initialize");
	opaque_pixmap = XCreatePixmap(top_level->display,
		top_level->win, w, h,
		top_level->default_depth);
	opaque_xft_draw = XftDrawCreate(top_level->display,
		opaque_pixmap,
		top_level->vis,
		top_level->cmap);

	if(mode == PIXMAP_ALPHA)
	{
		unsigned long gcmask = GCGraphicsExposures |
			GCForeground | GCBackground | GCFunction;
		XGCValues gcvalues;

		gcvalues.graphics_exposures = 0;        // prevent expose events for every redraw
		gcvalues.foreground = 0;
		gcvalues.background = 1;
		gcvalues.function = GXcopy;

		alpha_pixmap = XCreatePixmap(top_level->display, top_level->win,
			w, h, 1);

		alpha_gc = XCreateGC(top_level->display, top_level->win,
			gcmask, &gcvalues);

		copy_gc = XCreateGC(top_level->display, alpha_pixmap,
			gcmask, &gcvalues);

		alpha_xft_draw = XftDrawCreateBitmap(top_level->display,
			alpha_pixmap);
	}
	top_level->unlock_window();
}

void BC_Pixmap::resize(int w, int h)
{
	top_level->lock_window("BC_Pixmap::resize");
	Pixmap new_pixmap = XCreatePixmap(top_level->display, top_level->win,
		w, h, top_level->default_depth);

	XftDraw *new_xft_draw = XftDrawCreate(top_level->display,
		new_pixmap, top_level->vis, top_level->cmap);

	XCopyArea(top_level->display, opaque_pixmap, new_pixmap,
		top_level->gc, 0, 0, get_w(), get_h(), 0, 0);
	this->w = w;
	this->h = h;

	XftDrawDestroy((XftDraw*)opaque_xft_draw);
	XFreePixmap(top_level->display, opaque_pixmap);

	opaque_pixmap = new_pixmap;

	opaque_xft_draw = new_xft_draw;

	top_level->unlock_window();
}


void BC_Pixmap::copy_area(int x, int y, int w, int h, int x2, int y2)
{
	XCopyArea(top_level->display, opaque_pixmap, opaque_pixmap,
		top_level->gc, x, y, w, h, x2, y2);
}

void BC_Pixmap::write_drawable(Drawable &pixmap,
	int dest_x, int dest_y,
	int dest_w, int dest_h,
	int src_x, int src_y)
{
	if(dest_w < 0)
	{
		dest_w = w;
		src_x = 0;
	}

	if(dest_h < 0)
	{
		dest_h = h;
		src_y = 0;
	}

	top_level->lock_window("BC_Pixmap::write_drawable");
	if(mode == PIXMAP_ALPHA)
	{
		XSetClipOrigin(top_level->display, alpha_gc,
			dest_x - src_x, dest_y - src_y);
		XCopyArea(top_level->display,  opaque_pixmap,
			pixmap, alpha_gc, src_x, src_y, dest_w, dest_h,
			dest_x, dest_y);
	}
	else
		XCopyArea(top_level->display, opaque_pixmap,
			pixmap, top_level->gc, src_x, src_y, dest_w, dest_h,
			dest_x, dest_y);
	top_level->unlock_window();
}

void BC_Pixmap::draw_vframe(VFrame *frame, int dest_x, int dest_y,
	int dest_w, int dest_h, int src_x, int src_y)
{
	parent_window->draw_vframe(frame, dest_x, dest_y, dest_w, dest_h,
		src_x, src_y, 0, 0, this);
}

void BC_Pixmap::draw_pixmap(BC_Pixmap *pixmap, int dest_x, int dest_y,
	int dest_w, int dest_h, int src_x, int src_y)
{
	pixmap->write_drawable(this->opaque_pixmap,
		dest_x, dest_y,
		dest_w, dest_h,
		src_x, src_y);
}

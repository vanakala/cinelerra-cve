// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BCPIXMAP_H
#define BCPIXMAP_H

#include <X11/Xlib.h>

#include "config.h"

#include <X11/Xft/Xft.h>

#ifdef HAVE_GL
#include <GL/glx.h>
#endif

#include "bcbitmap.inc"
#include "bcpixmap.inc"
#include "bcwindowbase.inc"
#include "vframe.inc"

class BC_Pixmap
{
public:
	BC_Pixmap(BC_WindowBase *parent_window, VFrame *frame,
		int mode = PIXMAP_OPAQUE, int icon_offset = 0);
	BC_Pixmap(BC_WindowBase *parent_window, 
		int w, int h);
	~BC_Pixmap();

	friend class BC_WindowBase;

	void reset();
	void resize(int w, int h);
	void copy_area(int x, int y, int w, int h, int x2, int y2);
// Draw this pixmap onto the drawable pointed to by pixmap.
	void write_drawable(Drawable &pixmap,
		int dest_x, int dest_y,
		int dest_w = -1, int dest_h = -1,
		int src_x = -1, int src_y = -1);
// Draw the pixmap pointed to by pixmap onto this pixmap.
	void draw_pixmap(BC_Pixmap *pixmap,
		int dest_x = 0, int dest_y = 0,
		int dest_w = -1, int dest_h = -1,
		int src_x = 0, int src_y = 0);
// Draw the vframe pointed to by frame onto this pixmap.
	void draw_vframe(VFrame *frame,
		int dest_x = 0, int dest_y = 0,
		int dest_w = -1, int dest_h = -1,
		int src_x = 0, int src_y = 0);
	int get_w();
	int get_h();
	Pixmap get_pixmap();
	Pixmap get_alpha();

private:
	void initialize(BC_WindowBase *parent_window, int w, int h, int mode);

	BC_WindowBase *parent_window;
	BC_WindowBase *top_level;
	Pixmap opaque_pixmap, alpha_pixmap;
	void *opaque_xft_draw, *alpha_xft_draw;
	int w, h;
	int mode;
// GC's only used if alpha pixmap
	GC alpha_gc, copy_gc;
};

#endif

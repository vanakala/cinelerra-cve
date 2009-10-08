
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

#ifndef BCPIXMAP_H
#define BCPIXMAP_H

#include <X11/Xlib.h>

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#ifdef HAVE_XFT
#include <X11/Xft/Xft.h>
#endif

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
	BC_Pixmap(BC_WindowBase *parent_window, 
		VFrame *frame, 
		int mode = PIXMAP_OPAQUE,
		int icon_offset = 0);
	BC_Pixmap(BC_WindowBase *parent_window, 
		int w, 
		int h);
	~BC_Pixmap();

	friend class BC_WindowBase;

	void reset();
	void resize(int w, int h);
// OpenGL for pixmaps doesn't seem to be accelerated so it has been discontinued.
	void enable_opengl();
	void copy_area(int x, int y, int w, int h, int x2, int y2);
// Draw this pixmap onto the drawable pointed to by pixmap.
	int write_drawable(Drawable &pixmap,
			int dest_x, 
			int dest_y,
			int dest_w = -1,
			int dest_h = -1, 
			int src_x = -1,
			int src_y = -1);
// Draw the pixmap pointed to by pixmap onto this pixmap.
	void draw_pixmap(BC_Pixmap *pixmap, 
		int dest_x = 0, 
		int dest_y = 0, 
		int dest_w = -1, 
		int dest_h = -1,
		int src_x = 0,
		int src_y = 0);
// Draw the vframe pointed to by frame onto this pixmap.
	void draw_vframe(VFrame *frame, 
			int dest_x = 0, 
			int dest_y = 0, 
			int dest_w = -1, 
			int dest_h = -1,
			int src_x = 0,
			int src_y = 0);
	int get_w();
	int get_h();
	int get_w_fixed();
	int get_h_fixed();
	Pixmap get_pixmap();
	Pixmap get_alpha();
	int use_alpha();
	int use_opaque();

private:
	int initialize(BC_WindowBase *parent_window, int w, int h, int mode);

	BC_WindowBase *parent_window;
	BC_WindowBase *top_level;
	Pixmap opaque_pixmap, alpha_pixmap;
	void *opaque_xft_draw, *alpha_xft_draw;
#ifdef HAVE_GL
	GLXContext gl_pixmap_context;
	GLXPixmap gl_pixmap;
#endif
	int w, h;
	int mode;
// GC's only used if alpha pixmap
	GC alpha_gc, copy_gc;
};


#endif

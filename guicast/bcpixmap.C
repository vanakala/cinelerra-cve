
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

#include "bcbitmap.h"
#include "bcpixmap.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcsynchronous.h"
#include "bcwindowbase.h"
#include "vframe.h"


#include <unistd.h>

BC_Pixmap::BC_Pixmap(BC_WindowBase *parent_window, 
	VFrame *frame, 
	int mode,
	int icon_offset)
{
	reset();

	BC_Bitmap *opaque_bitmap, *alpha_bitmap, *mask_bitmap;
	if(frame->get_color_model() != BC_RGBA8888 &&
		mode == PIXMAP_ALPHA)
		mode = PIXMAP_OPAQUE;
	this->mode = mode;

// Temporary bitmaps
	if(use_opaque())
	{
		opaque_bitmap = new BC_Bitmap(parent_window, 
					frame->get_w(), 
					frame->get_h(), 
					parent_window->get_color_model(), 
					0);
		opaque_bitmap->set_bg_color(parent_window->get_bg_color());
		opaque_bitmap->read_frame(frame, 
			0, 
			0, 
			frame->get_w(), 
			frame->get_h());
		
	}

	if(use_alpha())
	{
		alpha_bitmap = new BC_Bitmap(parent_window, 
				frame->get_w(), 
				frame->get_h(), 
				BC_TRANSPARENCY, 
				0);

		if(frame->get_color_model() != BC_RGBA8888)
			printf("BC_Pixmap::BC_Pixmap: PIXMAP_ALPHA but frame doesn't have alpha.\n");
		alpha_bitmap->read_frame(frame, 
			0, 
			0, 
			frame->get_w(), 
			frame->get_h());
	}

	initialize(parent_window, 
		frame->get_w(), 
		frame->get_h(), 
		mode);

	if(use_opaque())
	{
		opaque_bitmap->write_drawable(opaque_pixmap, 
								top_level->gc,
								0, 
								0, 
								0, 
								0, 
								w, 
								h, 
								1);
		delete opaque_bitmap;
	}

	if(use_alpha())
	{
		alpha_bitmap->write_drawable(alpha_pixmap, 
			copy_gc, 
			0, 
			0, 
			icon_offset ? 2 : 0, 
			icon_offset ? 2 : 0, 
			w, 
			h, 
			1);
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
	if(use_opaque())
	{
#ifdef HAVE_XFT
		if(opaque_xft_draw)
			XftDrawDestroy((XftDraw*)opaque_xft_draw);
#endif
		XFreePixmap(top_level->display, opaque_pixmap);
	}

	if(use_alpha())
	{
		XFreeGC(top_level->display, alpha_gc);
#ifdef HAVE_XFT
		if(alpha_xft_draw)
			XftDrawDestroy((XftDraw*)alpha_xft_draw);
#endif
		XFreePixmap(top_level->display, alpha_pixmap);
	}


// Have to delete GL objects because pixmaps are deleted during resizes.
#ifdef HAVE_GL
	if(BC_WindowBase::get_synchronous() && gl_pixmap)
	{
		BC_WindowBase::get_synchronous()->delete_pixmap(parent_window,
			gl_pixmap,
			gl_pixmap_context);
	}
#endif
}


void BC_Pixmap::reset()
{
	parent_window = 0;
	top_level = 0;
	opaque_pixmap = 0;
	alpha_pixmap = 0;
	opaque_xft_draw = 0;
	alpha_xft_draw = 0;
#ifdef HAVE_GL
	gl_pixmap_context = 0;
	gl_pixmap = 0;
#endif
}

int BC_Pixmap::initialize(BC_WindowBase *parent_window, int w, int h, int mode)
{
	this->w = w;
	this->h = h;
	this->parent_window = parent_window;
	this->mode = mode;
	top_level = parent_window->top_level;

	if(use_opaque())
	{
		opaque_pixmap = XCreatePixmap(top_level->display, 
			top_level->win, 
			w, 
			h, 
			top_level->default_depth);
#ifdef HAVE_XFT
		if(BC_WindowBase::get_resources()->use_xft)
		{
			opaque_xft_draw = XftDrawCreate(top_level->display,
		    	   opaque_pixmap,
		    	   top_level->vis,
		    	   top_level->cmap);
		}
#endif
	}

	if(use_alpha())
	{
		unsigned long gcmask = GCGraphicsExposures | 
			GCForeground | 
			GCBackground | 
			GCFunction;
		XGCValues gcvalues;
		gcvalues.graphics_exposures = 0;        // prevent expose events for every redraw
		gcvalues.foreground = 0;
		gcvalues.background = 1;
		gcvalues.function = GXcopy;

		alpha_pixmap = XCreatePixmap(top_level->display, 
			top_level->win, 
			w, 
			h, 
			1);

		alpha_gc = XCreateGC(top_level->display, 
			top_level->win, 
			gcmask, 
			&gcvalues);

		copy_gc = XCreateGC(top_level->display,
			alpha_pixmap,
			gcmask,
			&gcvalues);

#ifdef HAVE_XFT
		if(BC_WindowBase::get_resources()->use_xft)
		{
			alpha_xft_draw = XftDrawCreateBitmap(top_level->display,
				alpha_pixmap);
		}
#endif
	}



// // For some reason, this is required in 32 bit.
// #ifdef HAVE_XFT
// 	if(BC_WindowBase::get_resources()->use_xft)
// 		XSync(top_level->display, False);
// #endif
	
	return 0;
}

void BC_Pixmap::resize(int w, int h)
{
	Pixmap new_pixmap = XCreatePixmap(top_level->display, 
			top_level->win, 
			w, 
			h, 
			top_level->default_depth);
#ifdef HAVE_XFT
	XftDraw *new_xft_draw;
	if(BC_WindowBase::get_resources()->use_xft)
	{
		new_xft_draw = XftDrawCreate(top_level->display,
		       new_pixmap,
		       top_level->vis,
		       top_level->cmap);
	}
#endif




	XCopyArea(top_level->display,
		opaque_pixmap,
		new_pixmap,
		top_level->gc,
		0,
		0,
		get_w(),
		get_h(),
		0,
		0);
	this->w = w;
	this->h = h;
#ifdef HAVE_XFT
	if(BC_WindowBase::get_resources()->use_xft)
		XftDrawDestroy((XftDraw*)opaque_xft_draw);
#endif
	XFreePixmap(top_level->display, opaque_pixmap);

	opaque_pixmap = new_pixmap;
#ifdef HAVE_XFT
	if(BC_WindowBase::get_resources()->use_xft)
		opaque_xft_draw = new_xft_draw;
#endif
}


void BC_Pixmap::copy_area(int x, int y, int w, int h, int x2, int y2)
{
	XCopyArea(top_level->display,
		opaque_pixmap,
		opaque_pixmap,
		top_level->gc,
		x,
		y,
		w,
		h,
		x2,
		y2);
}

int BC_Pixmap::write_drawable(Drawable &pixmap, 
			int dest_x, 
			int dest_y,
			int dest_w,
			int dest_h,
			int src_x,
			int src_y)
{
//printf("BC_Pixmap::write_drawable 1\n");
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

	if(use_alpha())
	{
		XSetClipOrigin(top_level->display, alpha_gc, dest_x - src_x, dest_y - src_y);
		XCopyArea(top_level->display, 
			this->opaque_pixmap, 
			pixmap, 
			alpha_gc, 
			src_x, 
			src_y, 
			dest_w, 
			dest_h, 
			dest_x, 
			dest_y);
	}
	else
	if(use_opaque())
	{
		XCopyArea(top_level->display, 
			this->opaque_pixmap, 
			pixmap, 
			top_level->gc, 
			src_x, 
			src_y, 
			dest_w, 
			dest_h, 
			dest_x, 
			dest_y);
	}
//printf("BC_Pixmap::write_drawable 2\n");

	return 0;
}

void BC_Pixmap::draw_vframe(VFrame *frame, 
		int dest_x, 
		int dest_y, 
		int dest_w, 
		int dest_h,
		int src_x,
		int src_y)
{
	parent_window->draw_vframe(frame, 
		dest_x, 
		dest_y, 
		dest_w, 
		dest_h,
		src_x,
		src_y,
		0,
		0,
		this);
}

void BC_Pixmap::draw_pixmap(BC_Pixmap *pixmap, 
	int dest_x, 
	int dest_y,
	int dest_w,
	int dest_h,
	int src_x,
	int src_y)
{
	pixmap->write_drawable(this->opaque_pixmap,
			dest_x, 
			dest_y,
			dest_w,
			dest_h,
			src_x,
			src_y);
}











int BC_Pixmap::get_w()
{
	return w;
}

int BC_Pixmap::get_h()
{
	return h;
}

int BC_Pixmap::get_w_fixed()
{
	return w - 1;
}

int BC_Pixmap::get_h_fixed()
{
	return h - 1;
}

Pixmap BC_Pixmap::get_pixmap()
{
	return opaque_pixmap;
}

Pixmap BC_Pixmap::get_alpha()
{
	return alpha_pixmap;
}

int BC_Pixmap::use_opaque()
{
	return 1;
}

int BC_Pixmap::use_alpha()
{
	return mode == PIXMAP_ALPHA;
}


void BC_Pixmap::enable_opengl()
{
printf("BC_Pixmap::enable_opengl called but it doesn't work.\n");
#ifdef HAVE_GL
	BC_WindowBase *current_window = BC_WindowBase::get_synchronous()->current_window;
	if(!gl_pixmap_context)
	{
		
// 		XVisualInfo viproto;
// 		int nvi;
// 		viproto.screen = current_window->get_screen();
// 		static int framebuffer_attributes[] = 
// 		{
//         	GLX_RGBA,
//         	GLX_RED_SIZE, 1,
//         	GLX_GREEN_SIZE, 1,
//         	GLX_BLUE_SIZE, 1,
// 			None
// 		};
// 		XVisualInfo *visinfo = glXChooseVisual(current_window->get_display(),
// 			current_window->get_screen(),
// 			framebuffer_attributes);
// printf("BC_Pixmap::enable_opengl 1 %p\n", visinfo->visual);
// 		gl_pixmap = glXCreateGLXPixmap(current_window->get_display(),
//             visinfo,
//             opaque_pixmap);
// printf("BC_Pixmap::enable_opengl 2 %p\n", gl_pixmap);
// // This context must be indirect, but because it's indirect, it can't share
// // ID's with the window context.
// 		gl_pixmap_context = glXCreateContext(current_window->get_display(),
// 			visinfo,
// 			0,
// 			0);
// printf("BC_Pixmap::enable_opengl 3 %p\n", gl_pixmap_context);

		static int framebuffer_attributes[] = 
		{
        	GLX_RENDER_TYPE, GLX_RGBA_BIT,
			GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT,
			GLX_DOUBLEBUFFER, 0,
        	GLX_RED_SIZE, 1,
        	GLX_GREEN_SIZE, 1,
        	GLX_BLUE_SIZE, 1,
			None
		};
		XVisualInfo *visinfo = 0;
		int config_result_count = 0;
		GLXFBConfig *config_result = glXChooseFBConfig(current_window->get_display(), 
			current_window->get_screen(), 
			framebuffer_attributes, 
			&config_result_count);
		if(config_result)
		{
			gl_pixmap = glXCreatePixmap(current_window->get_display(),
				config_result[0],
				opaque_pixmap,
				0);

			visinfo = glXGetVisualFromFBConfig(current_window->get_display(), 
				config_result[0]);
		}

// provide window context to share ID's.
		if(visinfo)
		{
			gl_pixmap_context = glXCreateContext(current_window->get_display(),
					visinfo,
					0,
					0);
		}

		if(config_result) XFree(config_result);
    	if(visinfo) XFree(visinfo);
	}

	if(gl_pixmap_context)
	{
		glXMakeCurrent(top_level->display,
			gl_pixmap,
			gl_pixmap_context);
	}
#endif
}


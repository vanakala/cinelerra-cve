#include "bcbitmap.h"
#include "bcpixmap.h"
#include "bcwindowbase.h"
#include "vframe.h"

BC_Pixmap::BC_Pixmap(BC_WindowBase *parent_window, 
	VFrame *frame, 
	int mode,
	int icon_offset)
{
	BC_Bitmap *opaque_bitmap, *alpha_bitmap, *mask_bitmap;
	this->mode = mode;

// Temporary bitmaps
	if(use_opaque())
	{
		opaque_bitmap = new BC_Bitmap(parent_window, 
					frame->get_w(), 
					frame->get_h(), 
					parent_window->get_color_model(), 
					1);
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
				1);

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
	initialize(parent_window, w, h, PIXMAP_OPAQUE);
}


BC_Pixmap::~BC_Pixmap()
{
	if(use_opaque())
	{
		XFreePixmap(top_level->display, opaque_pixmap);
	}

	if(use_alpha())
	{
		XFreeGC(top_level->display, alpha_gc);
		XFreePixmap(top_level->display, alpha_pixmap);
	}
}

int BC_Pixmap::initialize(BC_WindowBase *parent_window, int w, int h, int mode)
{
//printf("BC_Pixmap::initialize 1\n");
	unsigned long gcmask = GCGraphicsExposures | GCForeground | GCBackground | GCFunction;
	XGCValues gcvalues;
	gcvalues.graphics_exposures = 0;        // prevent expose events for every redraw
	gcvalues.foreground = 0;
	gcvalues.background = 1;
	gcvalues.function = GXcopy;

//printf("BC_Pixmap::initialize 1\n");
	this->w = w;
	this->h = h;
	this->parent_window = parent_window;
	this->mode = mode;
	top_level = parent_window->top_level;

//printf("BC_Pixmap::initialize 1\n");
	if(use_opaque())
	{
		opaque_pixmap = XCreatePixmap(top_level->display, 
			top_level->win, 
			w, 
			h, 
			top_level->default_depth);
	}

//printf("BC_Pixmap::initialize 1\n");
	if(use_alpha())
	{
		alpha_pixmap = XCreatePixmap(top_level->display, 
			top_level->win, 
			w, 
			h, 
			1);

//printf("BC_Pixmap::initialize 1\n");
		alpha_gc = XCreateGC(top_level->display, 
			top_level->win, 
			gcmask, 
			&gcvalues);

//printf("BC_Pixmap::initialize 1\n");
		copy_gc = XCreateGC(top_level->display,
			alpha_pixmap,
			gcmask,
			&gcvalues);
	}
	
//printf("BC_Pixmap::initialize 2\n");
	return 0;
}

void BC_Pixmap::resize(int w, int h)
{
	Pixmap new_pixmap = XCreatePixmap(top_level->display, 
			top_level->win, 
			w, 
			h, 
			top_level->default_depth);
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
	XFreePixmap(top_level->display, opaque_pixmap);
	opaque_pixmap = new_pixmap;
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

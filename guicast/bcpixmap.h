#ifndef BCPIXMAP_H
#define BCPIXMAP_H

#include <X11/Xlib.h>
#ifdef HAVE_XFT
#include <X11/Xft/Xft.h>
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

	void resize(int w, int h);
	void copy_area(int x, int y, int w, int h, int x2, int y2);
	int write_drawable(Drawable &pixmap,
			int dest_x, 
			int dest_y,
			int dest_w = -1,
			int dest_h = -1, 
			int src_x = -1,
			int src_y = -1);
	void draw_vframe(VFrame *frame, 
			int dest_x = 0, 
			int dest_y = 0, 
			int dest_w = -1, 
			int dest_h = -1,
			int src_x = 0,
			int src_y = 0);
	void draw_pixmap(BC_Pixmap *pixmap, 
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
//#ifdef HAVE_XFT
//	XftDraw *opaque_xft_draw, *alpha_xft_draw;
//#endif
	void *opaque_xft_draw, *alpha_xft_draw;
	int w, h;
	int mode;
	GC alpha_gc, copy_gc;
};


#endif

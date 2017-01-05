
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

#ifndef BCBITMAP_H
#define BCBITMAP_H

#include <X11/Xlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xvlib.h>

#include "bcwindowbase.inc"
#include "bcdisplayinfo.h"
#include "colors.h"
#include "vframe.inc"

//#define BITMAP_RING 1
#define BITMAP_RING 4

class BC_Bitmap
{
public:
	BC_Bitmap(BC_WindowBase *parent_window, unsigned char *png_data);
	BC_Bitmap(BC_WindowBase *parent_window, VFrame *frame);

// Shared memory is a problem in X because it's asynchronous and there's
// no easy way to join with the blitting process.
	BC_Bitmap(BC_WindowBase *parent_window, 
		int w, 
		int h, 
		int color_model, 
		int use_shm = 1);
	virtual ~BC_Bitmap();

// transfer VFrame
	void read_frame(VFrame *frame,
		int in_x, int in_y, int in_w, int in_h,
		int out_x, int out_y, int out_w, int out_h,
		int need_shm = -1, int cmodel = -1);
// x1, y1, x2, y2 dimensions of output area
	void read_frame(VFrame *frame, 
		int x1, int y1, int x2, int y2,
		int need_shm = -1, int cmodel = -1);

// Test if bitmap already matches parameters
	int params_match(int w, int h, int color_model, int use_shm);

// When showing the same frame twice need to rewind
	void rewind_ring();
// If dont_wait is true, the XSync comes before the flash.
// For YUV bitmaps, the image is scaled to fill dest_x ... w * dest_y ... h
	void write_drawable(Drawable &pixmap, 
			GC &gc,
			int source_x, 
			int source_y, 
			int source_w,
			int source_h,
			int dest_x, 
			int dest_y, 
			int dest_w, 
			int dest_h, 
			int dont_wait);
	void write_drawable(Drawable &pixmap, 
			GC &gc,
			int dest_x, 
			int dest_y, 
			int source_x, 
			int source_y, 
			int dest_w, 
			int dest_h, 
			int dont_wait);
// the bitmap must be wholly contained in the source during a GetImage
	void read_drawable(Drawable &pixmap, int source_x, int source_y);

	int rotate_90(int side);
	int get_w();
	int get_h();
// Data pointers for current ring buffer
	unsigned char* get_data();
	unsigned char* get_y_plane();
	unsigned char* get_u_plane();
	unsigned char* get_v_plane();
// Get the frame buffer itself
	int get_color_model();
	int hardware_scaling();
	unsigned char** get_row_pointers();
	int get_bytes_per_line();

// Returns just the plane offset
	long get_y_offset();
	long get_u_offset();
	long get_v_offset();

// Rewing ringbuffer to the previous frame
	void rewind_ringbuffer();
	void set_bg_color(int color);
	void invert();

	void dump(int minmax = 0);

private:
	void initialize(BC_WindowBase *parent_window, int w, int h, int color_model, int use_shm);
	void allocate_data();
	void delete_data();
	int get_default_depth();
	char byte_bitswap(char src);

	int ring_buffers, current_ringbuffer;
	int w, h;
// Color model from colormodels.h
	int color_model;
// Background color for using pngs
	int bg_color;
// Override top_level for small bitmaps
	int use_shm;
	BC_WindowBase *top_level;
	BC_WindowBase *parent_window;
// Points directly to the frame buffer
	unsigned char *data[BITMAP_RING];
// Row pointers to the frame buffer
	unsigned char **row_data[BITMAP_RING];
	int xv_portid;
// This differs from the depth parameter of top_level
	int bits_per_pixel;
// From the ximage
	long bytes_per_line;
// For resetting XVideo
	int last_pixmap_used;
// Background color
	unsigned char bg_r, bg_g, bg_b;
// For less than 8 bit depths
	int bit_counter;
// Area to show from bitmap
	int base_left;
	int base_top;
	int disp_w;
	int disp_h;

// X11 objects
// Need last pixmap to stop XVideo
	Drawable last_pixmap;
	XImage *ximage[BITMAP_RING];
	XvImage *xv_image[BITMAP_RING];
	XShmSegmentInfo shm_info;
};

#endif

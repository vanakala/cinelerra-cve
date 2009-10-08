
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

#ifndef BCSCROLLBAR_H
#define BCSCROLLBAR_H

#include "bclistbox.inc"
#include "bcsubwindow.h"

// Orientations
#define SCROLL_HORIZ 0
#define SCROLL_VERT  1

// Selection identifiers
#define SCROLL_HANDLE 1
#define SCROLL_BACKPAGE 2
#define SCROLL_FWDPAGE 3
#define SCROLL_BACKARROW 4
#define SCROLL_FWDARROW 5

// Image identifiers
#define SCROLL_HANDLE_UP 	0
#define SCROLL_HANDLE_HI 	1
#define SCROLL_HANDLE_DN 	2
#define SCROLL_HANDLE_BG 	3
#define SCROLL_BACKARROW_UP 4
#define SCROLL_BACKARROW_HI 5
#define SCROLL_BACKARROW_DN 6
#define SCROLL_FWDARROW_UP  7
#define SCROLL_FWDARROW_HI  8
#define SCROLL_FWDARROW_DN  9
#define SCROLL_IMAGES 		10









class BC_ScrollBar : public BC_SubWindow
{
public:
	BC_ScrollBar(int x, 
		int y, 
		int orientation, 
		int pixels, 
		int64_t length, 
		int64_t position, 
		int64_t handlelength,
		VFrame **data = 0);
	virtual ~BC_ScrollBar();

	friend class BC_ListBox;

	virtual int handle_event() { return 0; };
	int initialize();
	int cursor_motion_event();
	int cursor_leave_event();
	int cursor_enter_event();
	int button_press_event();
	int button_release_event();
	int repeat_event(int64_t repeat_id);
	int64_t get_value();
	int64_t get_position();
	int64_t get_length();
	int64_t get_handlelength();
	int get_pixels();
	void set_images(VFrame **data);
	int in_use();
	int update_value(int64_t value);
	int update_length(int64_t length, int64_t position, int64_t handlelength);
	int reposition_window(int x, int y, int pixels);
	int get_span();
	static int get_span(int orientation);
	int get_arrow_pixels();

private:
	void calculate_dimensions(int &w, int &h);
	int activate();
	void draw();
	void get_handle_dimensions();
	int get_cursor_zone(int cursor_x, int cursor_y);

	int64_t length, position, handlelength;   // handle position and size
	int selection_status, highlight_status;
	int orientation, pixels;
	int handle_pixel, handle_pixels;
	int min_pixel, max_pixel;
	int64_t repeat_count;
// Don't deactivate if bound to another tool
	BC_WindowBase *bound_to;
	VFrame **data;
	BC_Pixmap *images[SCROLL_IMAGES];
};









#endif

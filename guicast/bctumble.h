
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

#ifndef BCTUMBLE_H
#define BCTUMBLE_H

#include "bcsubwindow.h"

class BC_Tumbler : public BC_SubWindow
{
public:
	BC_Tumbler(int x, int y, VFrame **data = 0);
	virtual ~BC_Tumbler();

	virtual int handle_up_event() { return 0; };
	virtual int handle_down_event() { return 0; };
	int repeat_event(int64_t repeat_id);

	int initialize();
	int set_images(VFrame **data);
	int cursor_enter_event();
	int cursor_leave_event();
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	int update_bitmaps(VFrame **data);
	int reposition_window(int x, int y, int w=-1, int h=-1); // w & h don't do anything, except to inherit BC_Subwindow::(reposition_window)
	virtual void set_boundaries(int64_t min, int64_t max) {};
	virtual void set_boundaries(float min, float max) {};
	virtual void set_increment(float value) {};
	virtual void set_log_floatincrement(int value) {};

private:
	int draw_face();

	BC_Pixmap *images[4];
	int status;
	int64_t repeat_count;
	VFrame **data;
};

class BC_ITumbler : public BC_Tumbler
{
public:
	BC_ITumbler(BC_TextBox *textbox, int64_t min, int64_t max, int x, int y);
	virtual ~BC_ITumbler();

	int handle_up_event();
	int handle_down_event();
	void set_increment(float value);
	void set_boundaries(int64_t min, int64_t max);

	int64_t min, max;
	int64_t increment;
	BC_TextBox *textbox;
};

class BC_FTumbler : public BC_Tumbler
{
public:
	BC_FTumbler(BC_TextBox *textbox, float min, float max, int x, int y);
	virtual ~BC_FTumbler();
	
	int handle_up_event();
	int handle_down_event();
	void set_boundaries(float min, float max);
	void set_increment(float value);
	void set_log_floatincrement(int value);

	float min, max;
	float increment;
	int log_floatincrement;
	BC_TextBox *textbox;
};

#endif

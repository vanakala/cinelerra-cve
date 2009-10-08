
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

#ifndef BCTOGGLE_H
#define BCTOGGLE_H

#include "bcbitmap.inc"
#include "bcsubwindow.h"
#include "colors.h"
#include "fonts.h"
#include "vframe.inc"


class BC_Toggle : public BC_SubWindow
{
public:
	BC_Toggle(int x, int y, 
		VFrame **data,
		int value, 
		char *caption = "",
		int bottom_justify = 0,
		int font = MEDIUMFONT,
		int color = -1);
	virtual ~BC_Toggle();

	virtual int handle_event() { return 0; };
	int get_value();
	int set_value(int value, int draw = 1);
	void set_select_drag(int value);
	int update(int value, int draw = 1);
	void reposition_window(int x, int y);
	void enable();
	void disable();
	void set_status(int value);
	static void calculate_extents(BC_WindowBase *gui, 
		VFrame **images,
		int bottom_justify,
		int *text_line,
		int *w,
		int *h,
		int *toggle_x,
		int *toggle_y,
		int *text_x,
		int *text_y, 
		int *text_w,
		int *text_h, 
		char *caption);

	int initialize();
	int set_images(VFrame **data);
	void set_underline(int number);
	int cursor_enter_event();
	int cursor_leave_event();
// In select drag mode these 3 need to be overridden and called back to.
	virtual int button_press_event();
	virtual int button_release_event();
	int cursor_motion_event();
	int repeat_event(int64_t repeat_id);
	int draw_face();

	enum
	{
		TOGGLE_UP,
		TOGGLE_UPHI,
		TOGGLE_CHECKED,
		TOGGLE_DOWN,
		TOGGLE_CHECKEDHI
	};

	int has_caption();

	BC_Pixmap *images[5];
	BC_Pixmap *bg_image;
	VFrame **data;
	char caption[BCTEXTLEN];
	int status;
	int value;
	int toggle_x;
	int toggle_y;
// Start of text background
	int text_x;
// Start of text
	int text_text_x;
	int text_y;
// Width of text background
	int text_w;
	int text_h;
	int text_line;
	int bottom_justify;
	int font;
	int color;
	int select_drag;
	int enabled;
	int underline;
	int is_radial;
};

class BC_Radial : public BC_Toggle
{
public:
	BC_Radial(int x, 
		int y, 
		int value, 
		char *caption = "", 
		int font = MEDIUMFONT,
		int color = -1);
};

class BC_CheckBox : public BC_Toggle
{
public:
	BC_CheckBox(int x, 
		int y, 
		int value, 
		char *caption = "", 
		int font = MEDIUMFONT,
		int color = -1);
	BC_CheckBox(int x, 
		int y, 
		int *value, 
		char *caption = "", 
		int font = MEDIUMFONT,
		int color = -1);
	virtual int handle_event();


	int *value;
};

class BC_Label : public BC_Toggle
{
public:
	BC_Label(int x, 
		int y, 
		int value, 
		int font = MEDIUMFONT,
		int color = -1);
};

#endif

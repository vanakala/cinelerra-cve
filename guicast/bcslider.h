
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

#ifndef BCSLIDER_H
#define BCSLIDER_H

#include "bcbitmap.inc"
#include "bcsubwindow.h"

class BC_ISlider;
class BC_FSlider;
class BC_PercentageSlider;

class BC_Slider : public BC_SubWindow
{
public:
	BC_Slider(int x, 
		int y, 
		int pixels, 
		int pointer_motion_range,  
		VFrame **images,
		int show_number, 
		int vertical,
		int use_caption);
	virtual ~BC_Slider();

	friend class BC_ISlider;
	friend class BC_FSlider;
	friend class BC_PercentageSlider;

	virtual int handle_event() { return 0; };

	int initialize();
	static int get_span(int vertical);
	int get_button_pixels();
	virtual int value_to_pixel() { return 0; };
	int keypress_event();
	int cursor_enter_event();
	int cursor_leave_event();
	int button_press_event();
	virtual int button_release_event();
	int get_pointer_motion_range();
	int cursor_motion_event();
	int repeat_event(int64_t repeat_id);
	int reposition_window(int x, int y, int w = -1, int h = -1);
	int activate();
	int deactivate();
	virtual int increase_value() { return 0; };
	virtual int decrease_value() { return 0; };
	virtual int increase_value_big() { return 0; };
	virtual int decrease_value_big() { return 0; };
	virtual char* get_caption() { return caption; };

private:

#define SLIDER_UP 0
#define SLIDER_HI 1
#define SLIDER_DN 2
#define SLIDER_BG_UP 0
#define SLIDER_BG_HI 1
#define SLIDER_BG_DN 2
#define SLIDER_IMAGES 6

	virtual int init_selection(int cursor_x, int cursor_y) { return 0; };
	virtual int update_selection(int cursor_x, int cursor_y) { return 0; };
	int set_images(VFrame **images);
	int draw_face();
	void show_value_tooltip();

	VFrame **images;
	BC_Pixmap **pixmaps;
	int show_number, vertical, pointer_motion_range, pixels;
	int keypress_tooltip_timer;
	int button_pixel;
	int status;
	int button_down;
	int min_pixel, max_pixel;
	int text_line, text_height;
	int use_caption;
	char caption[BCTEXTLEN];
	char temp_tooltip_text[BCTEXTLEN];
	int active;
	int enabled;
};


class BC_ISlider : public BC_Slider
{
public:
	BC_ISlider(int x, 
			int y,
			int vertical,
			int pixels, 
			int pointer_motion_range, 
			int64_t minvalue, 
			int64_t maxvalue, 
			int64_t value,
			int use_caption = 0,
			VFrame **data = 0,
			int *output = 0);

	int update(int64_t value);
	int update(int pointer_motion_range, int64_t value, int64_t minvalue, int64_t maxvalue);
	int64_t get_value();
	int64_t get_length();
	int increase_value();
	int decrease_value();
	int increase_value_big();
	int decrease_value_big();
	virtual int handle_event();
	virtual char* get_caption();

private:
	int value_to_pixel();
	int init_selection(int cursor_x, int cursor_y);
	int update_selection(int cursor_x, int cursor_y);
	int64_t minvalue, maxvalue, value;
	int *output;
};

class BC_FSlider : public BC_Slider
{
public:
	BC_FSlider(int x, 
			int y,
			int vertical,
			int pixels, 
			int pointer_motion_range, 
			float minvalue, 
			float maxvalue, 
			float value,
			int use_caption = 0,
			VFrame **data = 0);

	friend class BC_PercentageSlider;

	int update(float value);
	int update(int pointer_motion_range, float value, float minvalue, float maxvalue);
	float get_value();
	float get_length();
	virtual int increase_value();
	virtual int decrease_value();
	virtual int increase_value_big();
	virtual int decrease_value_big();
	virtual char* get_caption();
	void set_precision(float value);
	void set_pagination(float small_change, float big_change);

private:
	int value_to_pixel();
	int init_selection(int cursor_x, int cursor_y);
	int update_selection(int cursor_x, int cursor_y);
	float minvalue, maxvalue, value;
	float precision;
	float small_change, big_change;
};

class BC_PercentageSlider : public BC_FSlider
{
public:
	BC_PercentageSlider(int x, 
			int y,
			int vertical,
			int pixels, 
			int pointer_motion_range, 
			float minvalue, 
			float maxvalue, 
			float value,
			int use_caption = 0,
			VFrame **data = 0);

	char* get_caption();
private:
};


#endif

// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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

	void initialize();
	static int get_span(int vertical);
	int get_button_pixels();
	virtual int value_to_pixel() { return 0; };
	int keypress_event();
	int cursor_enter_event();
	void cursor_leave_event();
	int button_press_event();
	virtual int button_release_event();
	int get_pointer_motion_range();
	void set_pointer_motion_range(int value);
	int cursor_motion_event();
	void repeat_event(int repeat_id);
	void reposition_window(int x, int y, int w = -1, int h = -1);
	void activate();
	void deactivate();
	virtual void increase_value() {};
	virtual void decrease_value() {};
	virtual void increase_value_big() {};
	virtual void decrease_value_big() {};
	virtual char* get_caption() { return caption; };

private:

#define SLIDER_UP 0
#define SLIDER_HI 1
#define SLIDER_DN 2
#define SLIDER_BG_UP 0
#define SLIDER_BG_HI 1
#define SLIDER_BG_DN 2
#define SLIDER_IMAGES 6

	virtual void init_selection(int cursor_x, int cursor_y) {};
	virtual int update_selection(int cursor_x, int cursor_y) { return 0; };
	void set_images(VFrame **images);
	void draw_face();
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
		int minvalue,
		int maxvalue,
		int value,
		int use_caption = 0,
		VFrame **data = 0,
		int *output = 0);

	void update(int value);
	void update(int pointer_motion_range, int value,
		int minvalue, int maxvalue);
	int get_value();
	int get_length();
	void increase_value();
	void decrease_value();
	void increase_value_big();
	void decrease_value_big();
	virtual int handle_event();
	virtual char* get_caption();

private:
	int value_to_pixel();
	void init_selection(int cursor_x, int cursor_y);
	int update_selection(int cursor_x, int cursor_y);
	int minvalue, maxvalue, value;
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
		double minvalue,
		double maxvalue,
		double value,
		int use_caption = 0,
		VFrame **data = 0);

	friend class BC_PercentageSlider;

	void update(double value);
	void update(int pointer_motion_range, double value,
		double minvalue, double maxvalue);
	double get_value();
	double get_length();
	virtual void increase_value();
	virtual void decrease_value();
	virtual void increase_value_big();
	virtual void decrease_value_big();
	virtual char *get_caption();
	void set_precision(double value);
	void set_pagination(double small_change, double big_change);

private:
	int value_to_pixel();
	void init_selection(int cursor_x, int cursor_y);
	int update_selection(int cursor_x, int cursor_y);
	double minvalue, maxvalue, value;
	double precision;
	double small_change, big_change;
};

class BC_PercentageSlider : public BC_FSlider
{
public:
	BC_PercentageSlider(int x, 
		int y,
		int vertical,
		int pixels,
		int pointer_motion_range,
		double minvalue,
		double maxvalue,
		double value,
		int use_caption = 0,
		VFrame **data = 0);

	char* get_caption();
};

#endif

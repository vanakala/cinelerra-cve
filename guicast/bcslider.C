// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcpixmap.h"
#include "bcresources.h"
#include "bcslider.h"
#include "clip.h"
#include "colors.h"
#include "fonts.h"
#include "keys.h"
#include "units.h"
#include "vframe.h"
#include <inttypes.h>
#include <wctype.h>

#include <string.h>


BC_Slider::BC_Slider(int x,
	int y,
	int pixels,
	int pointer_motion_range,
	VFrame **images,
	int show_number,
	int vertical,
	int use_caption)
 : BC_SubWindow(x, y, 0, 0, -1)
{
	this->images = images;
	this->show_number = show_number;
	this->vertical = vertical;
	this->pointer_motion_range = pointer_motion_range;
	this->pixels = pixels;
	this->button_pixel = button_pixel;
	this->use_caption = use_caption;

	status = SLIDER_UP;
	pixmaps = new BC_Pixmap*[SLIDER_IMAGES];
	for(int i = 0; i < SLIDER_IMAGES; i++)
	{
		pixmaps[i] = 0;
	}
	keypress_tooltip_timer = 0;
	button_down = 0;
	enabled = 1;
	active = 0;
}

BC_Slider::~BC_Slider()
{
	for(int i = 0; i < SLIDER_IMAGES; i++)
	{
		if(pixmaps[i]) delete pixmaps[i];
	}
	if(pixmaps) delete [] pixmaps;
}

void BC_Slider::initialize()
{
	if(!images)
	{
		this->images = vertical ? 
			resources.vertical_slider_data :
			resources.horizontal_slider_data;
	}

	set_images(images);

	if(vertical)
	{
		w = images[SLIDER_BG_UP]->get_w();
		h = pixels;
	}
	else
	{
		w = pixels;
		h = images[SLIDER_BG_UP]->get_h();
	}

	text_height = get_text_height(SMALLFONT);
	button_pixel = value_to_pixel();

	BC_SubWindow::initialize();
	draw_face();
}

int BC_Slider::get_span(int vertical)
{
	if(vertical)
	{
		return resources.vertical_slider_data[0]->get_w();
	}
	else
	{
		return resources.horizontal_slider_data[0]->get_h();
	}
}

void BC_Slider::draw_face()
{
// Clear background
	top_level->lock_window("BC_Slider::draw_face");
	draw_top_background(parent_window, 0, 0, get_w(), get_h());

	if(vertical)
	{
		draw_3segmentv(0, 
			0, 
			get_h(),
			pixmaps[SLIDER_IMAGES / 2 + status]);
		draw_pixmap(pixmaps[status], 0, button_pixel);
		if(use_caption) 
		{
			set_color(RED);
			set_font(SMALLFONT);
			draw_text(0, h, get_caption());
		}
	}
	else
	{
		int y = get_h() / 2 - pixmaps[SLIDER_IMAGES / 2 + status]->get_h() / 2;
		draw_3segmenth(0, 
			0, 
			get_w(),
			pixmaps[SLIDER_IMAGES / 2 + status]);
		draw_pixmap(pixmaps[status], button_pixel, 0);
		if(use_caption)
		{
			set_color(RED);
			set_font(SMALLFONT);
			draw_text(0, h, get_caption());
		}
	}

	flash();
	top_level->unlock_window();
}

void BC_Slider::set_images(VFrame **images)
{
	for(int i = 0; i < SLIDER_IMAGES; i++)
	{
		if(pixmaps[i]) delete pixmaps[i];
		pixmaps[i] = new BC_Pixmap(parent_window, images[i], PIXMAP_ALPHA);
	}
}

int BC_Slider::get_button_pixels()
{
	return vertical ? pixmaps[SLIDER_UP]->get_h() : 
		pixmaps[SLIDER_UP]->get_w();
}

void BC_Slider::show_value_tooltip()
{
	set_tooltip(get_caption());
	keypress_tooltip_timer = 2000;
	show_tooltip(50);
}

void BC_Slider::repeat_event(int duration)
{
	if(duration == resources.tooltip_delay)
	{
		if(tooltip_on)
		{
			if(keypress_tooltip_timer > 0)
			{
				keypress_tooltip_timer -= resources.tooltip_delay;
			}
			else
			if(status != SLIDER_HI && status != SLIDER_DN)
			{
				hide_tooltip();
			}
		}
		else
		if(status == SLIDER_HI)
		{
			if(!tooltip_wtext || iswdigit(tooltip_wtext[0]))
			{
				set_tooltip(get_caption());
				show_tooltip(50);
			}
			else
			{
				set_tooltip(get_caption());
				show_tooltip();
			}
			tooltip_done = 1;
		}
	}
}


int BC_Slider::keypress_event()
{
	int result = 0;
	if(!active || !enabled) return 0;
	if(ctrl_down() || shift_down()) return 0;

	switch(get_keypress())
	{
	case UP:
		increase_value_big();
		result = 1;
		break;
	case DOWN:
		decrease_value_big();
		result = 1;
		break;
	case LEFT:
		decrease_value();
		result = 1;
		break;
	case RIGHT:
		increase_value();
		result = 1;
		break;
	}

	if(result)
	{
		handle_event();
		show_value_tooltip();
		draw_face();
	}
	return result;
}

int BC_Slider::cursor_enter_event()
{
	if(top_level->event_win == win && status == SLIDER_UP)
	{
		tooltip_done = 0;
		status = SLIDER_HI;
		draw_face();
	}
	return 0;
}

void BC_Slider::cursor_leave_event()
{
	if(status == SLIDER_HI)
	{
		status = SLIDER_UP;
		draw_face();
		hide_tooltip();
	}
}

void BC_Slider::deactivate()
{
	active = 0;
}

void BC_Slider::activate()
{
	top_level->active_subwindow = this;
	active = 1;
}

int BC_Slider::button_press_event()
{
	int result = 0;
	if(is_event_win())
	{
		if(!tooltip_on) top_level->hide_tooltip();
		if(status == SLIDER_HI)
		{
			if(get_buttonpress() == 4)
			{
				increase_value();
				handle_event();
				show_value_tooltip();
				draw_face();
			}
			else
			if(get_buttonpress() == 5)
			{
				decrease_value();
				handle_event();
				show_value_tooltip();
				draw_face();
			}
			else
			if(get_buttonpress() == 1)
			{
				button_down = 1;
				status = SLIDER_DN;
				draw_face();
				init_selection(top_level->cursor_x, top_level->cursor_y);
				top_level->deactivate();
				activate();
				show_value_tooltip();
			}
			result = 1;
		}
	}
	return result;
}

int BC_Slider::button_release_event()
{
	if(button_down) 
	{
		button_down = 0;
		if(cursor_inside()) 
			status = SLIDER_HI;
		else
		{
			status = SLIDER_UP;
			top_level->hide_tooltip();
		}
		draw_face();
		return 1;
	}
	return 0;
}

int BC_Slider::cursor_motion_event()
{
	if(button_down)
	{
		int old_pixel = button_pixel;
		int result = update_selection(top_level->cursor_x, top_level->cursor_y);
		if(button_pixel != old_pixel) draw_face();
		if(result) 
		{
			handle_event();
			set_tooltip(get_caption());
		}
		return 1;
	}
	return 0;
}

void BC_Slider::reposition_window(int x, int y, int w, int h)
{
	BC_SubWindow::reposition_window(x, y, w, h);
	button_pixel = value_to_pixel();
	draw_face();
}


int BC_Slider::get_pointer_motion_range()
{
	return pointer_motion_range;
}

void BC_Slider::set_pointer_motion_range(int value)
{
	pointer_motion_range = value;
}


BC_ISlider::BC_ISlider(int x,
	int y,
	int vertical,
	int pixels,
	int pointer_motion_range,
	int minvalue,
	int maxvalue,
	int value,
	int use_caption,
	VFrame **data,
	int *output)
 : BC_Slider(x,
	y,
	pixels,
	pointer_motion_range,
	data,
	1,
	vertical,
	use_caption)
{
	this->minvalue = minvalue;
	this->maxvalue = maxvalue;
	this->value = value;
	this->output = output;
}

int BC_ISlider::value_to_pixel()
{
	if(maxvalue == minvalue)
		return 0;
	else
	{
		if(vertical)
			return round((1.0 - (double)(value - minvalue) / (maxvalue - minvalue)) *
				(get_h() - get_button_pixels()));
		else
			return round((double)(value - minvalue) / (maxvalue - minvalue) *
				(get_w() - get_button_pixels()));
	}
}

void BC_ISlider::update(int value)
{
	if(this->value != value)
	{
		this->value = value;
		int old_pixel = button_pixel;
		button_pixel = value_to_pixel();
		if(button_pixel != old_pixel) draw_face();
	}
}

void BC_ISlider::update(int pointer_motion_range,
	int value,
	int minvalue,
	int maxvalue)
{
	this->minvalue = minvalue;
	this->maxvalue = maxvalue;
	this->value = value;
	this->pointer_motion_range = pointer_motion_range;

	int old_pixel = button_pixel;
	button_pixel = value_to_pixel();
	if(button_pixel != old_pixel) draw_face();
}

int BC_ISlider::get_value()
{
	return value;
}

int BC_ISlider::get_length()
{
	return maxvalue - minvalue;
}

char *BC_ISlider::get_caption()
{
	sprintf(caption, "%d", value);
	return caption;
}

void BC_ISlider::increase_value()
{
	value++;
	if(value > maxvalue) value = maxvalue;
	button_pixel = value_to_pixel();
}

void BC_ISlider::decrease_value()
{
	value--;
	if(value < minvalue) value = minvalue;
	button_pixel = value_to_pixel();
}

void BC_ISlider::increase_value_big()
{
	value += 10;
	if(value > maxvalue) value = maxvalue;
	button_pixel = value_to_pixel();
}

void BC_ISlider::decrease_value_big()
{
	value -= 10;
	if(value < minvalue) value = minvalue;
	button_pixel = value_to_pixel();
}

void BC_ISlider::init_selection(int cursor_x, int cursor_y)
{
	if(vertical)
	{
		min_pixel = -round((1.0 - (double)(value - minvalue) / (maxvalue - minvalue)) * pointer_motion_range);
		min_pixel += cursor_y;
	}
	else
	{
		min_pixel = -round((double)(value - minvalue) / (maxvalue - minvalue) * pointer_motion_range);
		min_pixel += cursor_x;
	}
	max_pixel = min_pixel + pointer_motion_range;
}

int BC_ISlider::update_selection(int cursor_x, int cursor_y)
{
	int old_value = value;

	if(vertical)
	{
		value = round((1.0 - (double)(cursor_y - min_pixel) /
				pointer_motion_range) *
				(maxvalue - minvalue) +
				minvalue);
	}
	else
	{
		value = round((double)(cursor_x - min_pixel) /
				pointer_motion_range *
				(maxvalue - minvalue) +
				minvalue);
	}

	if(value > maxvalue) value = maxvalue;
	if(value < minvalue) value = minvalue;
	button_pixel = value_to_pixel();

	if(old_value != value)
	{
		return 1;
	}
	return 0;
}

int BC_ISlider::handle_event()
{
	if(output) *output = get_value();
	return 1;
}


BC_FSlider::BC_FSlider(int x, 
	int y,
	int vertical,
	int pixels,
	int pointer_motion_range,
	double minvalue,
	double maxvalue,
	double value,
	int use_caption,
	VFrame **data)
 : BC_Slider(x, 
	y,
	pixels,
	pointer_motion_range,
	data,
	1,
	vertical,
	use_caption)
{
	this->minvalue = minvalue;
	this->maxvalue = maxvalue;
	this->value = value;
	this->precision = 0.1;
	this->small_change = 0.1;
	this->big_change = 1.0;
}

int BC_FSlider::value_to_pixel()
{
	if(maxvalue == minvalue) return 0;
	{
		if(vertical)
			return round((1.0 - (double)(value - minvalue) / (maxvalue - minvalue)) *
				(get_h() - get_button_pixels()));
		else
			return round((double)(value - minvalue) / (maxvalue - minvalue) *
				(get_w() - get_button_pixels()));
	}
}

void BC_FSlider::update(double value)
{
	if(!EQUIV(this->value, value))
	{
		this->value = value;
		int old_pixel = button_pixel;
		button_pixel = value_to_pixel();
		if(button_pixel != old_pixel) draw_face();
	}
}

void BC_FSlider::update(int pointer_motion_range, double value,
	double minvalue, double maxvalue)
{
	this->minvalue = minvalue;
	this->maxvalue = maxvalue;
	this->value = value;
	this->pointer_motion_range = pointer_motion_range;
	int old_pixel = button_pixel;
	button_pixel = value_to_pixel();
	if(button_pixel != old_pixel) draw_face();
}

double BC_FSlider::get_value()
{
	return value;
}

double BC_FSlider::get_length()
{
	return maxvalue - minvalue;
}

char *BC_FSlider::get_caption()
{
	sprintf(caption, "%.02f", value);
	return caption;
}

void BC_FSlider::increase_value()
{
	value += small_change;
	if(value > maxvalue) value = maxvalue;
	button_pixel = value_to_pixel();
}

void BC_FSlider::decrease_value()
{
	value -= small_change;
	if(value < minvalue) value = minvalue;
	button_pixel = value_to_pixel();
}

void BC_FSlider::increase_value_big()
{
	value += big_change;
	if(value > maxvalue) value = maxvalue;
	button_pixel = value_to_pixel();
}

void BC_FSlider::decrease_value_big()
{
	value -= big_change;
	if(value < minvalue) value = minvalue;
	button_pixel = value_to_pixel();
}

void BC_FSlider::init_selection(int cursor_x, int cursor_y)
{
	if(vertical)
	{
		min_pixel = -round((1.0 - (double)(value - minvalue) / (maxvalue - minvalue)) * pointer_motion_range);
		min_pixel += cursor_y;
	}
	else
	{
		min_pixel = -round((double)(value - minvalue) / (maxvalue - minvalue) * pointer_motion_range);
		min_pixel += cursor_x;
	}
	max_pixel = min_pixel + pointer_motion_range;
}

int BC_FSlider::update_selection(int cursor_x, int cursor_y)
{
	double old_value = value;

	if(vertical)
	{
		value = ((1.0 - (double)(cursor_y - min_pixel) / 
			pointer_motion_range) * 
			(maxvalue - minvalue) +
			minvalue);
	}
	else
	{
		value = ((double)(cursor_x - min_pixel) / 
			pointer_motion_range * 
			(maxvalue - minvalue) +
			minvalue);
	}

	value = Units::quantize(value, precision);
	if(value > maxvalue) value = maxvalue;
	if(value < minvalue) value = minvalue;
	button_pixel = value_to_pixel();

	return !EQUIV(old_value, value);
}

void BC_FSlider::set_precision(double value)
{
	this->precision = value;
}

void BC_FSlider::set_pagination(double small_change, double big_change)
{
	this->small_change = small_change;
	this->big_change = big_change;
}


BC_PercentageSlider::BC_PercentageSlider(int x, 
	int y,
	int vertical,
	int pixels,
	int pointer_motion_range,
	double minvalue,
	double maxvalue,
	double value,
	int use_caption,
	VFrame **data)
 : BC_FSlider(x, 
	y,
	vertical,
	pixels,
	pointer_motion_range,
	minvalue,
	maxvalue,
	value,
	use_caption,
	data)
{
}

char* BC_PercentageSlider::get_caption()
{
	double val = 0;

	if(maxvalue > minvalue)
		val = floor((value - minvalue) / (maxvalue - minvalue) * 100);
	sprintf(caption, "%.0f%%", val);
	return caption;
}

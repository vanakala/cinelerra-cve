
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

#include "bcpixmap.h"
#include "bcresources.h"
#include "bcslider.h"
#include "colors.h"
#include "fonts.h"
#include "keys.h"
#include "units.h"
#include "vframe.h"



#include <ctype.h>
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

int BC_Slider::initialize()
{
	if(!images)
	{
		this->images = vertical ? 
			BC_WindowBase::get_resources()->vertical_slider_data : 
			BC_WindowBase::get_resources()->horizontal_slider_data;
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
	return 0;
}

int BC_Slider::get_span(int vertical)
{
	if(vertical)
	{
		return BC_WindowBase::get_resources()->vertical_slider_data[0]->get_w();
	}
	else
	{
		return BC_WindowBase::get_resources()->horizontal_slider_data[0]->get_h();
	}
}

int BC_Slider::draw_face()
{
// Clear background
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
	return 0;
}

int BC_Slider::set_images(VFrame **images)
{
	for(int i = 0; i < SLIDER_IMAGES; i++)
	{
		if(pixmaps[i]) delete pixmaps[i];
		pixmaps[i] = new BC_Pixmap(parent_window, images[i], PIXMAP_ALPHA);
	}
	return 0;
}

int BC_Slider::get_button_pixels()
{
	return vertical ? pixmaps[SLIDER_UP]->get_h() : 
		pixmaps[SLIDER_UP]->get_w();
}

void BC_Slider::show_value_tooltip()
{
//printf("BC_Slider::show_value_tooltip %s\n", get_caption());
	set_tooltip(get_caption());
	keypress_tooltip_timer = 2000;
	show_tooltip(50);
}


int BC_Slider::repeat_event(int64_t duration)
{
	if(duration == top_level->get_resources()->tooltip_delay)
	{
		if(tooltip_on)
		{
			if(keypress_tooltip_timer > 0)
			{
				keypress_tooltip_timer -= get_resources()->tooltip_delay;
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
			if(!tooltip_text[0] || isdigit(tooltip_text[0]))
			{
				set_tooltip(get_caption());
				show_tooltip(50);
			}
			else
			{
//printf("BC_Slider::repeat_event 1 %s\n", tooltip_text);
				set_tooltip(get_caption());
				show_tooltip();
			}
			tooltip_done = 1;
			return 1;
		}
	}
	return 0;
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
//printf("BC_Slider::cursor_enter_event 1\n");
	if(top_level->event_win == win && status == SLIDER_UP)
	{
		tooltip_done = 0;
		status = SLIDER_HI;
		draw_face();
	}
//printf("BC_Slider::cursor_enter_event 2\n");
	return 0;
}

int BC_Slider::cursor_leave_event()
{
	if(status == SLIDER_HI)
	{
		status = SLIDER_UP;
		draw_face();
		hide_tooltip();
	}
	return 0;
}

int BC_Slider::deactivate()
{
	active = 0;
	return 0;
}

int BC_Slider::activate()
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

int BC_Slider::reposition_window(int x, int y, int w, int h)
{
	BC_SubWindow::reposition_window(x, y, w, h);
	button_pixel = value_to_pixel();
	draw_face();
	return 0;
}


int BC_Slider::get_pointer_motion_range()
{
	return pointer_motion_range;
}





BC_ISlider::BC_ISlider(int x, 
			int y,
			int vertical,
			int pixels, 
			int pointer_motion_range, 
			int64_t minvalue, 
			int64_t maxvalue, 
			int64_t value,
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
	if(maxvalue == minvalue) return 0;
	else
	{
		if(vertical)
			return (int)((1.0 - (double)(value - minvalue) / (maxvalue - minvalue)) * 
				(get_h() - get_button_pixels()));
		else
			return (int)((double)(value - minvalue) / (maxvalue - minvalue) * 
				(get_w() - get_button_pixels()));
	}
}

int BC_ISlider::update(int64_t value)
{
	if(this->value != value)
	{
		this->value = value;
		int old_pixel = button_pixel;
		button_pixel = value_to_pixel();
		if(button_pixel != old_pixel) draw_face();
	}
	return 0;
}

int BC_ISlider::update(int pointer_motion_range, 
	int64_t value, 
	int64_t minvalue, 
	int64_t maxvalue)
{
	this->minvalue = minvalue;
	this->maxvalue = maxvalue;
	this->value = value;
	this->pointer_motion_range = pointer_motion_range;

	int old_pixel = button_pixel;
	button_pixel = value_to_pixel();
	if(button_pixel != old_pixel) draw_face();
	return 0;
}


int64_t BC_ISlider::get_value()
{
	return value;
}

int64_t BC_ISlider::get_length()
{
	return maxvalue - minvalue;
}

char* BC_ISlider::get_caption()
{
	sprintf(caption, "%ld", value);
	return caption;
}

int BC_ISlider::increase_value()
{
	value++;
	if(value > maxvalue) value = maxvalue;
	button_pixel = value_to_pixel();
	return 0;
}

int BC_ISlider::decrease_value()
{
	value-=10;
	if(value < minvalue) value = minvalue;
	button_pixel = value_to_pixel();
	return 0;
}

int BC_ISlider::increase_value_big()
{
	value+=10;
	if(value > maxvalue) value = maxvalue;
	button_pixel = value_to_pixel();
	return 0;
}

int BC_ISlider::decrease_value_big()
{
	value--;
	if(value < minvalue) value = minvalue;
	button_pixel = value_to_pixel();
	return 0;
}

int BC_ISlider::init_selection(int cursor_x, int cursor_y)
{
	if(vertical)
	{
		min_pixel = -(int)((1.0 - (double)(value - minvalue) / (maxvalue - minvalue)) * pointer_motion_range);
		min_pixel += cursor_y;
	}
	else
	{
		min_pixel = -(int)((double)(value - minvalue) / (maxvalue - minvalue) * pointer_motion_range);
		min_pixel += cursor_x;
	}
	max_pixel = min_pixel + pointer_motion_range;
	return 0;
}

int BC_ISlider::update_selection(int cursor_x, int cursor_y)
{
	int64_t old_value = value;

	if(vertical)
	{
		value = (int64_t)((1.0 - (double)(cursor_y - min_pixel) / 
			pointer_motion_range) * 
			(maxvalue - minvalue) +
			minvalue);
	}
	else
	{
		value = (int64_t)((double)(cursor_x - min_pixel) / 
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
			float minvalue, 
			float maxvalue, 
			float value,
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
//printf("BC_FSlider::value_to_pixel %f %f\n", maxvalue, minvalue);
	if(maxvalue == minvalue) return 0;
	{
		if(vertical)
			return (int)((1.0 - (double)(value - minvalue) / (maxvalue - minvalue)) * 
				(get_h() - get_button_pixels()));
		else
			return (int)((double)(value - minvalue) / (maxvalue - minvalue) * 
				(get_w() - get_button_pixels()));
	}
}

int BC_FSlider::update(float value)
{
	if(this->value != value)
	{
		this->value = value;
		int old_pixel = button_pixel;
		button_pixel = value_to_pixel();
//printf("BC_FSlider::update 1 %f %d\n", value, button_pixel);
		if(button_pixel != old_pixel) draw_face();
	}
	return 0;
}

int BC_FSlider::update(int pointer_motion_range, float value, float minvalue, float maxvalue)
{
	this->minvalue = minvalue;
	this->maxvalue = maxvalue;
	this->value = value;
	this->pointer_motion_range = pointer_motion_range;
	int old_pixel = button_pixel;
	button_pixel = value_to_pixel();
	if(button_pixel != old_pixel) draw_face();
	return 0;
}


float BC_FSlider::get_value()
{
	return value;
}

float BC_FSlider::get_length()
{
	return maxvalue - minvalue;
}

char* BC_FSlider::get_caption()
{
	sprintf(caption, "%.02f", value);
	return caption;
}

int BC_FSlider::increase_value()
{
	value += small_change;
	if(value > maxvalue) value = maxvalue;
	button_pixel = value_to_pixel();
	return 0;
}

int BC_FSlider::decrease_value()
{
	value -= small_change;
	if(value < minvalue) value = minvalue;
	button_pixel = value_to_pixel();
	return 0;
}

int BC_FSlider::increase_value_big()
{
	value += big_change;
	if(value > maxvalue) value = maxvalue;
	button_pixel = value_to_pixel();
	return 0;
}

int BC_FSlider::decrease_value_big()
{
	value -= big_change;
	if(value < minvalue) value = minvalue;
	button_pixel = value_to_pixel();
	return 0;
}

int BC_FSlider::init_selection(int cursor_x, int cursor_y)
{
	if(vertical)
	{
		min_pixel = -(int)((1.0 - (double)(value - minvalue) / (maxvalue - minvalue)) * pointer_motion_range);
		min_pixel += cursor_y;
	}
	else
	{
		min_pixel = -(int)((double)(value - minvalue) / (maxvalue - minvalue) * pointer_motion_range);
		min_pixel += cursor_x;
	}
	max_pixel = min_pixel + pointer_motion_range;
	return 0;
}

int BC_FSlider::update_selection(int cursor_x, int cursor_y)
{
	float old_value = value;


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
// printf("BC_FSlider::update_selection 1 %d %d %d %d %f %f\n", 
// pointer_motion_range, 
// min_pixel,
// max_pixel,
// cursor_x, 
// precision,
// value);

	if(old_value != value)
	{
		return 1;
	}
	return 0;
}

void BC_FSlider::set_precision(float value)
{
	this->precision = value;
}

void BC_FSlider::set_pagination(float small_change, float big_change)
{
	this->small_change = small_change;
	this->big_change = big_change;
}



BC_PercentageSlider::BC_PercentageSlider(int x, 
			int y,
			int vertical,
			int pixels, 
			int pointer_motion_range, 
			float minvalue, 
			float maxvalue, 
			float value,
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
	sprintf(caption, "%.0f%%", floor((value - minvalue) / (maxvalue - minvalue) * 100));
	return caption;
}


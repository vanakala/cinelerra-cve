
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
#include "bctextbox.h"
#include "bctumble.h"
#include "math.h"


#define TUMBLE_UP 0
#define TUMBLE_UPHI 1
#define TUMBLEBOTTOM_DN 2
#define TUMBLETOP_DN 3
#define TOTAL_STATES 4

BC_Tumbler::BC_Tumbler(int x, int y, VFrame **data)
 : BC_SubWindow(x, y, 0, 0, -1)
{
	for(int i = 0; i < TOTAL_STATES; i++)
		images[i] = 0;
	status = TUMBLE_UP;
	repeat_count = 0;
	this->data = data;
}


BC_Tumbler::~BC_Tumbler()
{
	for(int i = 0; i < TOTAL_STATES; i ++)
		delete images[i];
}



int BC_Tumbler::initialize()
{
// Get the image
	if(data)
		set_images(data);
	else
		set_images(get_resources()->tumble_data);
	w = images[TUMBLE_UP]->get_w();
	h = images[TUMBLE_UP]->get_h();

// Create the subwindow
	BC_SubWindow::initialize();

// Display the bitmap
	draw_face();
	return 0;
}

int BC_Tumbler::reposition_window(int x, int y, int w, int h)
{
	if (w > 0 || h > 0) 
		printf("BC_Tumbler::reposition_window - w & h haven't been implemented yet!! (probably never will be)");

	BC_WindowBase::reposition_window(x, y);
	draw_face();
	return 0;
}


int BC_Tumbler::update_bitmaps(VFrame **data)
{
	set_images(data);
	draw_top_background(parent_window, 0, 0, w, h);
	draw_face();
	return 0;
}

int BC_Tumbler::set_images(VFrame **data)
{
	for(int i = 0; i < TOTAL_STATES; i++)
	{
		if(images[i]) delete images[i];
		images[i] = new BC_Pixmap(parent_window, data[i], PIXMAP_ALPHA);
	}

	return 0;
}

int BC_Tumbler::draw_face()
{
	draw_top_background(parent_window, 0, 0, w, h);
	pixmap->draw_pixmap(images[status], 
			0, 
			0,
			w,
			h,
			0,
			0);
	flash();
	return 0;
}

int BC_Tumbler::repeat_event(int64_t duration)
{
//printf("BC_Tumbler::repeat_event 1 %d\n", duration);
	if(duration == top_level->get_resources()->tooltip_delay)
	{
		if(tooltip_text[0] != 0 &&
			status == TUMBLE_UPHI &&
			!tooltip_done)
		{
			show_tooltip();
			tooltip_done = 1;
			return 1;
		}
	}
	else
	if(duration == top_level->get_resources()->tumble_duration)
	{
//printf("BC_Tumbler::repeat_event 2\n");
		repeat_count++;
		if(repeat_count == 2) return 0;
		if(status == TUMBLETOP_DN)
		{
			handle_up_event();
			return 1;
		}
		else
		if(status == TUMBLEBOTTOM_DN)
		{
			handle_down_event();
			return 1;
		}
	}
	return 0;
}

int BC_Tumbler::cursor_enter_event()
{
	if(top_level->event_win == win)
	{
		tooltip_done = 0;
		if(! top_level->button_down && status == TUMBLE_UP) 
		{
			status = TUMBLE_UPHI;
			draw_face();
		}
	}
	return 0;
}

int BC_Tumbler::cursor_leave_event()
{
	hide_tooltip();
	if(status == TUMBLE_UPHI)
	{
		status = TUMBLE_UP;
		draw_face();
	}
	return 0;
}

int BC_Tumbler::button_press_event()
{
	hide_tooltip();
	if(top_level->event_win == win)
	{
//printf("BC_Tumbler::button_press_event 1 %d\n", get_buttonpress());
		if(get_buttonpress() == 4)
		{
			status = TUMBLETOP_DN;
			draw_face();
			flush();
			handle_up_event();
//			repeat_count = 0;
//			repeat_event(top_level->get_resources()->tumble_duration);
		}
		else
		if(get_buttonpress() == 5)
		{
			status = TUMBLEBOTTOM_DN;
			draw_face();
			flush();
			handle_down_event();
//			repeat_count = 0;
//			repeat_event(top_level->get_resources()->tumble_duration);
		}
		else
		{
			if(top_level->cursor_y < get_h() / 2)
			{
				status = TUMBLETOP_DN;
			}
			else
			{
				status = TUMBLEBOTTOM_DN;
			}

			draw_face();
			flush();

			top_level->set_repeat(top_level->get_resources()->tumble_duration);
			repeat_count = 0;
			repeat_event(top_level->get_resources()->tumble_duration);
//printf("BC_Tumbler::button_press_event 2 %d\n", get_buttonpress());
		}
		return 1;
	}
	return 0;
}

int BC_Tumbler::button_release_event()
{
	hide_tooltip();
	if(top_level->event_win == win)
	{
		if(status == TUMBLEBOTTOM_DN || status == TUMBLETOP_DN)
		{
			top_level->unset_repeat(top_level->get_resources()->tumble_duration);
			if(cursor_inside())
				status = TUMBLE_UPHI;
			else
				status = TUMBLE_UP;
		}
		draw_face();
	}
	return 0;
}

int BC_Tumbler::cursor_motion_event()
{
	if(top_level->button_down && top_level->event_win == win && 
		!cursor_inside() &&
		!(status == TUMBLETOP_DN || status == TUMBLEBOTTOM_DN))
	{
		status = TUMBLE_UP;
		draw_face();
	}
	return 0;
}




BC_ITumbler::BC_ITumbler(BC_TextBox *textbox, int64_t min, int64_t max, int x, int y)
 : BC_Tumbler(x, y)
{
	this->textbox = textbox;
	this->min = min;
	this->max = max;
	this->increment = 1;
}

BC_ITumbler::~BC_ITumbler()
{
}

void BC_ITumbler::set_increment(float value)
{
	this->increment = (int64_t)value;
	if(increment < 1) increment = 1;
}

int BC_ITumbler::handle_up_event()
{
	int64_t value = atol(textbox->get_text());
	value += increment;
	if(value > max) value = max;
	textbox->update(value);
	textbox->handle_event();
	return 1;
}

int BC_ITumbler::handle_down_event()
{
	int64_t value = atol(textbox->get_text());
	value -= increment;
	if(value < min) value = min;
	textbox->update(value);
	textbox->handle_event();
	return 1;
}

void BC_ITumbler::set_boundaries(int64_t min, int64_t max)
{
	this->min = min;
	this->max = max;
}










BC_FTumbler::BC_FTumbler(BC_TextBox *textbox, 
	float min, 
	float max, 
	int x, 
	int y)
 : BC_Tumbler(x, y)
{
	this->textbox = textbox;
	this->min = min;
	this->max = max;
	this->increment = 1.0;
	this->log_floatincrement = 0;
}

BC_FTumbler::~BC_FTumbler()
{
}

int BC_FTumbler::handle_up_event()
{
	float value = atof(textbox->get_text());
	if (log_floatincrement) {
		// round off to to current precision (i.e. 250 -> 200)
		float cp = floor(log(value)/log(10) + 0.0001);
		value = floor((value/pow(10,cp))+ 0.0001)*pow(10,cp);
		value += pow(10,cp);
	}
	else
		value += increment;
	if(value > max) value = max;
	textbox->update(value);
	textbox->handle_event();
	return 1;
}

int BC_FTumbler::handle_down_event()
{
	float value = atof(textbox->get_text());
	if (log_floatincrement) {
		// round off to to current precision (i.e. 250 -> 200)
		float cp = floor(log(value)/log(10));
		value = floor(value/pow(10,cp))*pow(10,cp);
		// Need to make it so that: [.001 .01 .1 1 10 100] => [.0001 .001 .01 .1 1 10]
		cp = floor(log(value)/log(10)-.01);
		value -= pow(10,cp);
	}
	else
		value -= increment;
	if(value < min) value = min;
	textbox->update(value);
	textbox->handle_event();
	return 1;
}

void BC_FTumbler::set_boundaries(float min, float max)
{
	this->min = min;
	this->max = max;
}

void BC_FTumbler::set_increment(float value)
{
	this->increment = value;
}

void BC_FTumbler::set_log_floatincrement(int value)
{
	this->log_floatincrement = value;
}

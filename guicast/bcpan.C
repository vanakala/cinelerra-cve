
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

#include "bcpan.h"
#include "bcpixmap.h"
#include "bcpopup.h"
#include "bcresources.h"
#include "clip.h"
#include "colors.h"
#include "fonts.h"
#include "rotateframe.h"
#include "units.h"
#include "vframe.h"

#include <math.h>
#include <string.h>

BC_Pan::BC_Pan(int x, 
		int y, 
		int virtual_r, 
		float maxvalue, 
		int total_values, 
		int *value_positions, 
		int stick_x, 
		int stick_y,
		float *values)
 : BC_SubWindow(x, y, -1, -1, -1)
{
	this->virtual_r = virtual_r;
	this->maxvalue = maxvalue;
	this->total_values = total_values;
	this->values = new float[total_values];
	memcpy(this->values, values, sizeof(float) * total_values);
	this->value_positions = new int[total_values];
	memcpy(this->value_positions, value_positions, sizeof(int) * total_values);
	this->value_x = new int[total_values];
	this->value_y = new int[total_values];
	this->stick_x = stick_x;
	this->stick_y = stick_y;
	get_channel_positions(value_x, 
		value_y, 
		value_positions,
		virtual_r,
		total_values);
	if(stick_x < 0 || stick_y < 0)
		calculate_stick_position(total_values, 
			value_positions, 
			values, 
			maxvalue, 
			virtual_r,
			this->stick_x,
			this->stick_y);
	highlighted = 0;
	popup = 0;
	active = 0;
	memset(images, 0, sizeof(BC_Pixmap*) * PAN_IMAGES);
}

BC_Pan::~BC_Pan()
{
//printf("BC_Pan::~BC_Pan 1\n");
	delete [] values;
//printf("BC_Pan::~BC_Pan 1\n");
	delete [] value_positions;
//printf("BC_Pan::~BC_Pan 1\n");
	delete [] value_x;
//printf("BC_Pan::~BC_Pan 1\n");
	delete [] value_y;
//printf("BC_Pan::~BC_Pan 1\n");
	if(popup) delete popup;
//printf("BC_Pan::~BC_Pan 1\n");
	delete temp_channel;
//printf("BC_Pan::~BC_Pan 1\n");
	delete rotater;
	for(int i = 0; i < PAN_IMAGES; i++)
		if(images[i]) delete images[i];
//printf("BC_Pan::~BC_Pan 2\n");
}

int BC_Pan::initialize()
{
	set_images(get_resources()->pan_data);

	BC_SubWindow::initialize();
	temp_channel = new VFrame(0, 
		get_resources()->pan_data[PAN_CHANNEL]->get_w(),
		get_resources()->pan_data[PAN_CHANNEL]->get_h(),
		get_resources()->pan_data[PAN_CHANNEL]->get_color_model());
	rotater = new RotateFrame(1,
		get_resources()->pan_data[PAN_CHANNEL]->get_w(),
		get_resources()->pan_data[PAN_CHANNEL]->get_h());
	draw();
	return 0;
}

void BC_Pan::set_images(VFrame **data)
{
	for(int i = 0; i < PAN_IMAGES; i++)
	{
		if(images[i]) delete images[i];
		images[i] = new BC_Pixmap(parent_window, data[i], PIXMAP_ALPHA);
	}
	w = images[PAN_UP]->get_w();
	h = images[PAN_UP]->get_h();
}

int BC_Pan::button_press_event()
{
	// there are two modes of operation...
	if (popup)
	{	if (popup->is_event_win() && get_button_down() && get_buttonpress() == 1)
		{ 
			active = 1;
			x_origin = popup->get_cursor_x();
			y_origin = popup->get_cursor_y();
			stick_x_origin = stick_x;
			stick_y_origin = stick_y;
			return 1;
		} else
		{
			deactivate();
			return 0;
		}
	}
	if(is_event_win() && get_button_down() && get_buttonpress() == 1)
	{
		hide_tooltip();
		activate();
		active = 1;
		x_origin = get_cursor_x();
		y_origin = get_cursor_y();
		stick_x_origin = stick_x;
		stick_y_origin = stick_y;
		draw_popup();
		return 1;
	}
	return 0;
}

int BC_Pan::cursor_motion_event()
{
	if(popup && get_button_down() && get_buttonpress() == 1)
	{
		stick_x = stick_x_origin + get_cursor_x() - x_origin;
		stick_y = stick_y_origin + get_cursor_y() - y_origin;
		CLAMP(stick_x, 0, virtual_r * 2);
		CLAMP(stick_y, 0, virtual_r * 2);
		stick_to_values();
		draw_popup();
		handle_event();
		return 1;
	}
	return 0;
}

int BC_Pan::button_release_event()
{
	if(popup)
	{
		hide_tooltip();
		deactivate();
		draw();
		return 1;
	}
	return 0;
}

int BC_Pan::repeat_event(int64_t duration)
{
	if(duration == top_level->get_resources()->tooltip_delay &&
		tooltip_text[0] != 0 &&
		highlighted &&
		!active &&
		!tooltip_done)
	{
		show_tooltip();
		tooltip_done = 1;
		return 1;
	}
	return 0;
}

int BC_Pan::cursor_enter_event()
{
	if(is_event_win() && !highlighted)
	{
		tooltip_done = 0;
		highlighted = 1;
		draw();
	}
	return 0;
}

int BC_Pan::cursor_leave_event()
{
	if(highlighted)
	{
		highlighted = 0;
		hide_tooltip();
		draw();
	}
	return 0;
}



int BC_Pan::deactivate()
{
	if(popup) delete popup;
	popup = 0;
	active = 0;
	return 0;
}

int BC_Pan::activate(int popup_x, int popup_y)
{
	int x, y;
	Window tempwin;

	active = 0;
	if (popup_x < 0 || popup_y < 0)
	{
		XTranslateCoordinates(top_level->display, 
			win, 
			top_level->rootwin, 
			0, 
			0, 
			&x, 
			&y, 
			&tempwin);

		x -= (images[PAN_POPUP]->get_w() - get_w()) / 2;
		y -= (images[PAN_POPUP]->get_h() - get_h()) / 2;
		if (x < 0) x = 0;
	} else
	{
		XTranslateCoordinates(top_level->display, 
			top_level->win, 
			top_level->rootwin, 
			popup_x, 
			popup_y, 
			&x, 
			&y, 
			&tempwin);
		x -= images[PAN_POPUP]->get_w() / 2;
		y -= images[PAN_POPUP]->get_h() / 2;
		if (x < 0) x = 0;
	}
	
	
	if (popup) delete popup;
	popup = new BC_Popup(this, 
				x, 
				y, 
				images[PAN_POPUP]->get_w(), 
				images[PAN_POPUP]->get_h(), 
				0, 
				0, 
				images[PAN_POPUP]);
	draw_popup();
	flush();
	return 0;
}

int BC_Pan::update(int x, int y)
{
	if(x != stick_x ||
		y != stick_y)
	{
		stick_x = x;
		stick_y = y;
		stick_to_values();
		draw();
	}
	return 0;
}

void BC_Pan::draw_popup()
{
	popup->draw_background(0, 0, popup->get_w(), popup->get_h());

	int x1, y1;
	float rotate_angle;
	float scale = (float)(popup->get_w() - 
		get_resources()->pan_data[PAN_CHANNEL]->get_w()) / 
		(virtual_r * 2);
	set_color(get_resources()->pan_text_color);
	set_font(SMALLFONT);

	for(int i = 0; i < total_values; i++)
	{
		x1 = (int)(value_x[i] * scale);
		y1 = (int)(value_y[i] * scale);
		rotate_angle = value_positions[i];
		rotate_angle = -rotate_angle;
		while(rotate_angle < 0) rotate_angle += 360;
		rotater->rotate(temp_channel, 
			get_resources()->pan_data[PAN_CHANNEL], 
			rotate_angle, 
			0);
		BC_Pixmap *temp_pixmap = new BC_Pixmap(popup, 
			temp_channel, 
			PIXMAP_ALPHA);
		popup->draw_pixmap(temp_pixmap, x1, y1);
		delete temp_pixmap;

		char string[BCTEXTLEN];
		float value = values[i] + 0.005;
		sprintf(string, "%.1f", value);
		popup->draw_text(x1, y1 + get_text_height(SMALLFONT), string);
	}
	
	x1 = (int)(stick_x * scale);
	y1 = (int)(stick_y * scale);
	popup->draw_pixmap(images[PAN_STICK], x1, y1);
	popup->flash();
}

#define PICON_W 6
#define PICON_H 6

void BC_Pan::draw()
{
	draw_top_background(parent_window, 0, 0, w, h);
	
	draw_pixmap(images[highlighted ? PAN_HI : PAN_UP]);
	get_channel_positions(value_x, 
		value_y, 
		value_positions,
		virtual_r,
		total_values);

// draw channels
	int x1, y1, x2, y2, w, h, j;
	float scale = (float)(get_w() - PICON_W) / (virtual_r * 2);
	set_color(RED);

	for(int i = 0; i < total_values; i++)
	{
// printf("BC_Pan::draw 1 %d %d %d %d\n", 
// 	i, 
// 	value_positions[i], 
// 	value_x[i], 
// 	value_y[i]);
		x1 = (int)(value_x[i] * scale);
		y1 = (int)(value_y[i] * scale);
//printf("BC_Pan::draw 2 %d %d\n", x1, y1);
		CLAMP(x1, 0, get_w() - PICON_W);
		CLAMP(y1, 0, get_h() - PICON_H);
		draw_pixmap(images[PAN_CHANNEL_SMALL], x1, y1);
//		draw_box(x1, y1, PICON_W, PICON_H);
	}

// draw stick
 	set_color(GREEN);
 	x1 = (int)(stick_x * scale);
 	y1 = (int)(stick_y * scale);

//printf("BC_Pan::draw 2 %d %d\n", x1, y1);
	CLAMP(x1, 0, get_w() - PICON_W);
	CLAMP(y1, 0, get_h() - PICON_H);

	draw_pixmap(images[PAN_STICK_SMALL], x1, y1);
//  	x2 = x1 + PICON_W;
//  	y2 = y1 + PICON_H;
//  	draw_line(x1, y1, x2, y2);
//  	draw_line(x2, y1, x1, y2);

	flash();
}

int BC_Pan::stick_to_values()
{
	return stick_to_values(values,
		total_values, 
		value_positions, 
		stick_x, 
		stick_y,
		virtual_r,
		maxvalue);
}

int BC_Pan::stick_to_values(float *values,
		int total_values, 
		int *value_positions, 
		int stick_x, 
		int stick_y,
		int virtual_r,
		float maxvalue)
{
// find shortest distance to a channel
	float shortest = 2 * virtual_r, test_distance;
	int i;
	int *value_x = new int[total_values];
	int *value_y = new int[total_values];

	get_channel_positions(value_x, value_y, value_positions, virtual_r, total_values);
	for(i = 0; i < total_values; i++)
	{
		if((test_distance = distance(stick_x, 
			value_x[i], 
			stick_y, 
			value_y[i])) < shortest)
			shortest = test_distance;
	}

// get values for channels
	if(shortest == 0)
	{
		for(i = 0; i < total_values; i++)
		{
			if(distance(stick_x, value_x[i], stick_y, value_y[i]) == shortest)
				values[i] = maxvalue;
			else
				values[i] = 0;
		}
	}
	else
	{
		for(i = 0; i < total_values; i++)
		{
			values[i] = shortest;
			values[i] -= (float)(distance(stick_x, 
				value_x[i], 
				stick_y, 
				value_y[i]) - shortest);
			if(values[i] < 0) values[i] = 0;
			values[i] = values[i] / shortest * maxvalue;
		}
	}

	for(i = 0; i < total_values; i++)
	{
		values[i] = Units::quantize10(values[i]);
	}

	delete [] value_x;
	delete [] value_y;
	return 0;
}


float BC_Pan::distance(int x1, int x2, int y1, int y2)
{
	return hypot(x2 - x1, y2 - y1);
}

int BC_Pan::change_channels(int new_channels, int *value_positions)
{
	delete values;
	delete this->value_positions;
	delete value_x;
	delete value_y;
	
	values = new float[new_channels];
	this->value_positions = new int[new_channels];
	value_x = new int[new_channels];
	value_y = new int[new_channels];
	total_values = new_channels;
	for(int i = 0; i < new_channels; i++)
	{
		this->value_positions[i] = value_positions[i];
	}
	get_channel_positions(value_x, 
		value_y, 
		value_positions,
		virtual_r,
		total_values);
	stick_to_values();
	draw();
	return 0;
}

int BC_Pan::get_channel_positions(int *value_x, 
	int *value_y, 
	int *value_positions,
	int virtual_r,
	int total_values)
{
	for(int i = 0; i < total_values; i++)
	{
		rdtoxy(value_x[i], value_y[i], value_positions[i], virtual_r);
	}
	return 0;
}

int BC_Pan::get_total_values()
{
	return total_values;
}

float BC_Pan::get_value(int channel)
{
	return values[channel];
}

int BC_Pan::get_stick_x() 
{ 
	return stick_x; 
}

int BC_Pan::get_stick_y() 
{ 
	return stick_y; 
}

float* BC_Pan::get_values()
{
	return values;
}

int BC_Pan::rdtoxy(int &x, int &y, int a, int virtual_r)
{
	float radians = (float)a / 360 * 2 * M_PI;

	y = (int)(sin(radians) * virtual_r);
	x = (int)(cos(radians) * virtual_r);
	x += virtual_r;
	y = virtual_r - y;
	return 0;
}

void BC_Pan::calculate_stick_position(int total_values, 
	int *value_positions, 
	float *values, 
	float maxvalue, 
	int virtual_r,
	int &stick_x,
	int &stick_y)
{
// If 2 channels have positive values, use weighted average
	int channel1 = -1;
	int channel2 = -1;

	for(int i = 0; i < total_values; i++)
	{
		if(values[i] > 0.001)
		{
			if(channel1 < 0) channel1 = i;
			else
			if(channel2 < 0) channel2 = i;
			else
				break;
		}
	}

	if(channel1 >= 0 && channel2 >= 0)
	{
		int x1, y1, x2, y2;
		rdtoxy(x1, y1, value_positions[channel1], virtual_r);
		rdtoxy(x2, y2, value_positions[channel2], virtual_r);
		stick_x = (x1 + x2) / 2;
		stick_y = (y1 + y2) / 2;
	}
	else
	{

// use highest value as location of stick
		float highest_value = 0;
		int angle = 0;
		int i, j;

		for(i = 0; i < total_values; i++)
		{
			if(values[i] > highest_value)
			{
				highest_value = values[i];
				angle = value_positions[i];
			}
		}
		rdtoxy(stick_x, stick_y, angle, virtual_r);
	}

}


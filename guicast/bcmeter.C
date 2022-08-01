// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcbutton.h"
#include "bcmeter.h"
#include "bcpixmap.h"
#include "bcresources.h"
#include "bcwindow.h"
#include "colors.h"
#include "fonts.h"
#include "vframe.h"
#include <string.h>

// Which source image to replicate
#define METER_NORMAL 0
#define METER_GREEN 1
#define METER_RED 2
#define METER_YELLOW 3
#define METER_WHITE 4
#define METER_OVER 5

// Region of source image to replicate
#define METER_LEFT 0
#define METER_MID 1
#define METER_RIGHT 3


BC_Meter::BC_Meter(int x, 
	int y, 
	int orientation, 
	int pixels, 
	int min, 
	int max,
	int use_titles)
 : BC_SubWindow(x, y, -1, -1)
{
	this->use_titles = use_titles;
	this->min = min;
	this->max = max;
	this->orientation = orientation;
	this->pixels = pixels;
	for(int i = 0; i < TOTAL_METER_IMAGES; i++) images[i] = 0;
	db_titles.set_array_delete();
}

BC_Meter::~BC_Meter()
{
	db_titles.remove_all_objects();
	title_pixels.remove_all();
	tick_pixels.remove_all();
	for(int i = 0; i < TOTAL_METER_IMAGES; i++) delete images[i];
}

int BC_Meter::get_title_w()
{
	return resources.meter_title_w;
}

int BC_Meter::get_meter_w()
{
	return resources.ymeter_images[0]->get_w() + 2;
}


void BC_Meter::set_delays(int over_delay, int peak_delay)
{
	this->over_delay = over_delay;
	this->peak_delay = peak_delay;
}

void BC_Meter::initialize()
{
	peak_timer = 0;
	level_pixel = peak_pixel = 0;
	over_timer = 0;
	over_count = 0;
	peak = level = -100;

	if(orientation == METER_VERT)
	{
		set_images(resources.ymeter_images);
		h = pixels;
		w = images[0]->get_w();
		if(use_titles) w += get_title_w();
	}
	else
	{
		set_images(resources.xmeter_images);
		h = images[0]->get_h();
		w = pixels;
		if(use_titles) h += get_title_w();
	}

// calibrate the db titles
	get_divisions();

	BC_SubWindow::initialize();
	draw_titles();
	draw_face();
}

void BC_Meter::set_images(VFrame **data)
{
	for(int i = 0; i < TOTAL_METER_IMAGES; i++) delete images[i];
	for(int i = 0; i < TOTAL_METER_IMAGES; i++) 
		images[i] = new BC_Pixmap(parent_window, data[i], PIXMAP_ALPHA);
}

void BC_Meter::reposition_window(int x, int y, int pixels)
{
	if(pixels < 0) pixels = this->pixels;
	this->pixels = pixels;
	if(orientation == METER_VERT)
		BC_SubWindow::reposition_window(x, y, get_w(), pixels);
	else
		BC_SubWindow::reposition_window(x, y, pixels, get_h());

	get_divisions();

	draw_titles();
	draw_face();
}

void BC_Meter::reset()
{
	level = min;
	peak = min;
	level_pixel = peak_pixel = 0;
	peak_timer = 0;
	over_timer = 0;
	over_count = 0;
	draw_face();
}

int BC_Meter::button_press_event()
{
	if(cursor_inside() && is_event_win())
	{
		reset_over();
		return 1;
	}
	return 0;
}


void BC_Meter::reset_over()
{
	over_timer = 0;
}

void BC_Meter::change_format(int min, int max)
{
	this->min = min;
	this->max = max;
	reposition_window(get_x(), get_y(), pixels);
}

int BC_Meter::level_to_pixel(double level)
{
	int result;

	if(level <= min)
		return 0;
	else
	return round(pixels * (level - min) / (max - min));
}


void BC_Meter::get_divisions()
{
	int i;
	int j, j_step;
	int division, division_step;
	char string[BCTEXTLEN];
	char *new_string;

	db_titles.remove_all_objects();
	title_pixels.remove_all();
	tick_pixels.remove_all();

	low_division = 0;
	medium_division = 0;
	high_division = pixels;

	int current_pixel;
// Create tick marks and titles in one pass
	for(int current = min; current <= max; current++)
	{
		if(orientation == METER_VERT)
		{
// Create tick mark
			current_pixel = (pixels - METER_MARGIN * 2 - 2) * 
				(current - min) /
				(max - min) + 2;
			tick_pixels.append(current_pixel);

// Create titles in selected positions
			if(current == min || 
				current == max ||
				current == 0 ||
				(current - min > 4 && max - current > 4 && !(current % 5)))
			{
				int title_pixel = (pixels - 
					METER_MARGIN * 2) * 
					(current - min) /
					(max - min);
				sprintf(string, "%ld", labs(current));
				new_string = new char[strlen(string) + 1];
				strcpy(new_string, string);
				db_titles.append(new_string);
				title_pixels.append(title_pixel);
			}
		}
		else
		{
			current_pixel = (pixels - METER_MARGIN * 2) *
				(current - min) /
				(max - min);
			tick_pixels.append(current_pixel);
// Titles not supported for horizontal
		}

// Create color divisions
		if(current == -20)
		{
			low_division = current_pixel;
		}
		else
		if(current == -5)
		{
			medium_division = current_pixel;
		}
		else
		if(current == 0)
		{
			high_division = current_pixel;
		}
	}
}

void BC_Meter::draw_titles()
{
	if(!use_titles) return;

	top_level->lock_window("BC_Meter::draw_titles");
	set_font(resources.meter_font);

	if(orientation == METER_HORIZ)
	{
		draw_top_background(parent_window, 0, 0, get_w(), get_title_w());

		for(int i = 0; i < db_titles.total; i++)
		{
			draw_text(0, title_pixels.values[i], db_titles.values[i]);
		}

		flash(0, 0, get_w(), get_title_w());
	}
	else
	if(orientation == METER_VERT)
	{
		draw_top_background(parent_window, 0, 0, get_title_w(), get_h());

// Titles
		for(int i = 0; i < db_titles.total; i++)
		{
			int title_y = pixels - 
				title_pixels.values[i];
			if(i == 0) 
				title_y -= get_text_descent(SMALLFONT_3D);
			else
			if(i == db_titles.total - 1)
				title_y += get_text_ascent(SMALLFONT_3D);
			else
				title_y += get_text_ascent(SMALLFONT_3D) / 2;

			set_color(resources.meter_font_color);
			draw_text(0, 
				title_y,
				db_titles.values[i]);
		}

		for(int i = 0; i < tick_pixels.total; i++)
		{
// Tick marks
			int tick_y = pixels - tick_pixels.values[i] - METER_MARGIN;
			set_color(resources.meter_font_color);
			draw_line(get_title_w() - 10 - 1, tick_y, get_title_w() - 1, tick_y);
			if(resources.meter_3d)
			{
				set_color(BLACK);
				draw_line(get_title_w() - 10, tick_y + 1, get_title_w(), tick_y + 1);
			}
		}

		flash(0, 0, get_title_w(), get_h());
	}
	top_level->unlock_window();
}

int BC_Meter::region_pixel(int region)
{
	VFrame **reference_images = resources.xmeter_images;

	return region * reference_images[0]->get_w() / 4;
}

int BC_Meter::region_pixels(int region)
{
	int x1;
	int x2;
	int result;
	VFrame **reference_images = resources.xmeter_images;

	x1 = region * reference_images[0]->get_w() / 4;
	x2 = (region + 1) * reference_images[0]->get_w() / 4;
	if(region == METER_MID) 
		result = (x2 - x1) * 2;
	else 
		result = x2 - x1;
	return result;
}

void BC_Meter::draw_face()
{
	VFrame **reference_images = resources.xmeter_images;
	int level_pixel = level_to_pixel(level);
	int peak_pixel2 = level_to_pixel(peak);
	int peak_pixel1 = peak_pixel2 - 2;
	int left_pixel = region_pixel(METER_MID);
	int right_pixel = pixels - region_pixels(METER_RIGHT);
	int pixel = 0;
	int image_number = 0, region = 0;
	int in_span, in_start;
	int x = use_titles ? get_title_w() : 0;
	int w = use_titles ? this->w - get_title_w() : this->w;

	top_level->lock_window("BC_Meter::draw_face");
	draw_top_background(parent_window, x, 0, w, h);

	while(pixel < pixels)
	{
// Select image to draw
		if(pixel < level_pixel ||
			(pixel >= peak_pixel1 && pixel < peak_pixel2))
		{
			if(pixel < low_division)
				image_number = METER_GREEN;
			else
			if(pixel < medium_division)
				image_number = METER_YELLOW;
			else
			if(pixel < high_division)
				image_number = METER_RED;
			else
				image_number = METER_WHITE;
		}
		else
		{
			image_number = METER_NORMAL;
		}

// Select region of image to duplicate
		if(pixel < left_pixel)
		{
			region = METER_LEFT;
			in_start = pixel + region_pixel(region);
			in_span = region_pixels(region) - (in_start - region_pixel(region));
		}
		else
		if(pixel < right_pixel)
		{
			region = METER_MID;
			in_start = region_pixel(region);
			in_span = region_pixels(region);
		}
		else
		{
			region = METER_RIGHT;
			in_start = (pixel - right_pixel) + region_pixel(region);
			in_span = region_pixels(region) - (in_start - region_pixel(region));;
		}

		if(in_span > 0)
		{
// Clip length to peaks
			if(pixel < level_pixel && pixel + in_span > level_pixel)
				in_span = level_pixel - pixel;
			else
			if(pixel < peak_pixel1 && pixel + in_span > peak_pixel1)
				in_span = peak_pixel1 - pixel;
			else
			if(pixel < peak_pixel2 && pixel + in_span > peak_pixel2) 
				in_span = peak_pixel2 - pixel;

// Clip length to color changes
			if(image_number == METER_GREEN && pixel + in_span > low_division)
				in_span = low_division - pixel;
			else
			if(image_number == METER_YELLOW && pixel + in_span > medium_division)
				in_span = medium_division - pixel;
			else
			if(image_number == METER_RED && pixel + in_span > high_division)
				in_span = high_division - pixel;

// Clip length to regions
			if(pixel < left_pixel && pixel + in_span > left_pixel)
				in_span = left_pixel - pixel;
			else
			if(pixel < right_pixel && pixel + in_span > right_pixel)
				in_span = right_pixel - pixel;

			if(orientation == METER_HORIZ)
				draw_pixmap(images[image_number], 
					pixel, 
					x, 
					in_span + 1, 
					get_h(), 
					in_start, 
					0);
			else
				draw_pixmap(images[image_number],
					x,
					get_h() - pixel - in_span,
					get_w(),
					in_span + 1,
					0,
					images[image_number]->get_h() - in_start - in_span);

			pixel += in_span;
		}
		else
		{
// Sanity check
			break;
		}
	}

	if(over_timer)
	{
		if(orientation == METER_HORIZ)
			draw_pixmap(images[METER_OVER], 
				10, 
				2);
		else
			draw_pixmap(images[METER_OVER],
				x + 2,
				get_h() - 100);

		over_timer--;
	}

	if(orientation == METER_HORIZ)
		flash(0, 0, pixels, get_h());
	else
		flash(x, 0, w, pixels);

	top_level->unlock_window();
}

void BC_Meter::update(double new_value, int over)
{
	peak_timer++;

	if(new_value == 0) 
		level = min;
	else
		level = db.todb(new_value);        // db value

	if(level > peak || peak_timer > peak_delay)
	{
		peak = level;
		peak_timer = 0;
	}

	if(over) over_timer = over_delay;
// only draw if window is visible

	draw_face();
}

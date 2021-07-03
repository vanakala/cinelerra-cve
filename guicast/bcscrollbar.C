// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcpixmap.h"
#include "bcresources.h"
#include "bcscrollbar.h"
#include "clip.h"
#include "colors.h"
#include "vframe.h"

#include <string.h>

BC_ScrollBar::BC_ScrollBar(int x, int y, int orientation, int pixels,
	int length, int position, int handlelength,
	VFrame **data)
 : BC_SubWindow(x, y, 0, 0, -1)
{
	this->length = length;
	this->position = position;
	this->handlelength = handlelength;
	this->selection_status = 0;
	this->highlight_status = 0;
	this->orientation = orientation;
	this->pixels = pixels;

	if(data) 
		this->data = data;
	else
	if(orientation == SCROLL_HORIZ)
		this->data = resources.hscroll_data;
	else
		this->data = resources.vscroll_data;

	handle_pixel = 0;
	handle_pixels = 0;
	bound_to = 0;
	repeat_count = 0;
	memset(images, 0, sizeof(BC_Pixmap*) * SCROLL_IMAGES);
}

BC_ScrollBar::~BC_ScrollBar()
{
	for(int i = 0; i < SCROLL_IMAGES; i++)
		if(images[i]) delete images[i];
}

void BC_ScrollBar::initialize()
{
	set_images(data);

	BC_SubWindow::initialize();
	draw();
}

void BC_ScrollBar::set_images(VFrame **data)
{
	for(int i = 0; i < SCROLL_IMAGES; i++)
	{
		if(images[i]) delete images[i];
		images[i] = new BC_Pixmap(parent_window, data[i], PIXMAP_ALPHA);
	}
	calculate_dimensions(w, h);
}

void BC_ScrollBar::calculate_dimensions(int &w, int &h)
{
	switch(orientation)
	{
	case SCROLL_HORIZ:
		w = pixels;
		h = data[SCROLL_HANDLE_UP]->get_h();
		break;

	case SCROLL_VERT:
		w = data[SCROLL_HANDLE_UP]->get_w();
		h = pixels;
		break;
	}
}

int BC_ScrollBar::get_span(int orientation)
{
	switch(orientation)
	{
	case SCROLL_HORIZ:
		return resources.hscroll_data[SCROLL_HANDLE_UP]->get_h();

	case SCROLL_VERT:
		return resources.vscroll_data[SCROLL_HANDLE_UP]->get_w();
	}
	return 0;
}

int BC_ScrollBar::get_span()
{
	switch(orientation)
	{
	case SCROLL_HORIZ:
		return data[SCROLL_HANDLE_UP]->get_h();

	case SCROLL_VERT:
		return data[SCROLL_HANDLE_UP]->get_w();
	}
	return 0;
}

int BC_ScrollBar::get_arrow_pixels()
{
	switch(orientation)
	{
	case SCROLL_HORIZ:
		return data[SCROLL_BACKARROW_UP]->get_w();

	case SCROLL_VERT:
		return data[SCROLL_BACKARROW_UP]->get_h();
	}
	return 0;
}


void BC_ScrollBar::draw()
{
	top_level->lock_window("BC_ScrollBar::draw");
	draw_top_background(parent_window, 0, 0, w, h);
	get_handle_dimensions();

	switch(orientation)
	{
	case SCROLL_HORIZ:
// Too small to draw anything
		if(get_w() < get_arrow_pixels() * 2 + 5)
		{
			draw_3segmenth(0,
				0,
				get_w(),
				images[SCROLL_HANDLE_UP]);
		}
		else
		{
// back arrow
			if(selection_status == SCROLL_BACKARROW)
				draw_pixmap(images[SCROLL_BACKARROW_DN],
					0,
					0);
			else
			if(highlight_status == SCROLL_BACKARROW)
				draw_pixmap(images[SCROLL_BACKARROW_HI],
					0,
					0);
			else
				draw_pixmap(images[SCROLL_BACKARROW_UP],
					0,
					0);
// forward arrow
			if(selection_status == SCROLL_FWDARROW)
				draw_pixmap(images[SCROLL_FWDARROW_DN],
					get_w() - get_arrow_pixels(),
					0);
			else
			if(highlight_status == SCROLL_FWDARROW)
				draw_pixmap(images[SCROLL_FWDARROW_HI],
					get_w() - get_arrow_pixels(),
					0);
			else
				draw_pixmap(images[SCROLL_FWDARROW_UP],
					get_w() - get_arrow_pixels(),
					0);
// handle background
			draw_3segmenth(get_arrow_pixels(),
				0,
				handle_pixel - get_arrow_pixels(),
				images[SCROLL_HANDLE_BG]);
// handle foreground
			if(selection_status == SCROLL_HANDLE)
				draw_3segmenth(handle_pixel,
					0,
					handle_pixels,
					images[SCROLL_HANDLE_DN]);
			else
			if(highlight_status == SCROLL_HANDLE)
				draw_3segmenth(handle_pixel,
					0,
					handle_pixels,
					images[SCROLL_HANDLE_HI]);
			else
				draw_3segmenth(handle_pixel,
					0,
					handle_pixels,
					images[SCROLL_HANDLE_UP]);
// handle background
			draw_3segmenth(handle_pixel + handle_pixels,
				0,
				get_w() - get_arrow_pixels() - handle_pixel - handle_pixels,
				images[SCROLL_HANDLE_BG]);
			}
			break;

	case SCROLL_VERT:
// Too small to draw anything
		if(get_h() < get_arrow_pixels() * 2 + 5)
		{
			draw_3segmentv(0,
				0,
				get_w(),
				images[SCROLL_HANDLE_UP]);
		}
		else
		{
// back arrow
			if(selection_status == SCROLL_BACKARROW)
				draw_pixmap(images[SCROLL_BACKARROW_DN],
					0,
					0);
			else
			if(highlight_status == SCROLL_BACKARROW)
				draw_pixmap(images[SCROLL_BACKARROW_HI],
					0,
					0);
			else
				draw_pixmap(images[SCROLL_BACKARROW_UP],
					0,
					0);
// forward arrow
			if(selection_status == SCROLL_FWDARROW)
				draw_pixmap(images[SCROLL_FWDARROW_DN],
					0,
					get_h() - get_arrow_pixels());
			else
			if(highlight_status == SCROLL_FWDARROW)
				draw_pixmap(images[SCROLL_FWDARROW_HI],
					0,
					get_h() - get_arrow_pixels());
			else
				draw_pixmap(images[SCROLL_FWDARROW_UP],
					0,
					get_h() - get_arrow_pixels());
// handle background
			draw_3segmentv(0,
				get_arrow_pixels(),
				handle_pixel - get_arrow_pixels(),
				images[SCROLL_HANDLE_BG]);
// handle foreground
			if(selection_status == SCROLL_HANDLE)
				draw_3segmentv(0,
					handle_pixel,
					handle_pixels,
					images[SCROLL_HANDLE_DN]);
			else
			if(highlight_status == SCROLL_HANDLE)
				draw_3segmentv(0,
					handle_pixel,
					handle_pixels,
					images[SCROLL_HANDLE_HI]);
			else
				draw_3segmentv(0,
					handle_pixel,
					handle_pixels,
					images[SCROLL_HANDLE_UP]);
// handle background
			draw_3segmentv(0,
				handle_pixel + handle_pixels,
				get_h() - get_arrow_pixels() - handle_pixel - handle_pixels,
				images[SCROLL_HANDLE_BG]);
		}
		break;
	}
	flash();
	top_level->unlock_window();
}

void BC_ScrollBar::get_handle_dimensions()
{
	int total_pixels = pixels - 
		get_arrow_pixels() * 2;

	if(length > 0)
	{
		handle_pixels = round((double)handlelength / length *
			total_pixels);

		if(handle_pixels < resources.scroll_minhandle)
			handle_pixels = resources.scroll_minhandle;


		handle_pixel = round((double)position / length *
				total_pixels) + get_arrow_pixels();

// Handle pixels is beyond minimum right position.  Clamp it.
		if(handle_pixel > pixels - get_arrow_pixels() - resources.scroll_minhandle)
		{
			handle_pixel = pixels - get_arrow_pixels() - resources.scroll_minhandle;
			handle_pixels = resources.scroll_minhandle;
		}
// Shrink handle_pixels until it fits inside scrollbar
		if(handle_pixel > pixels - get_arrow_pixels() - handle_pixels)
		{
			handle_pixels = pixels - get_arrow_pixels() - handle_pixel;
		}
		if(handle_pixel < get_arrow_pixels())
		{
			handle_pixels = handle_pixel + handle_pixels - get_arrow_pixels();
			handle_pixel = get_arrow_pixels();
		}
		if(handle_pixels < resources.scroll_minhandle)
			handle_pixels = resources.scroll_minhandle;
	}
	else
	{
		handle_pixels = total_pixels;
		handle_pixel = get_arrow_pixels();
	}

	CLAMP(handle_pixel, get_arrow_pixels(), (int)(pixels - get_arrow_pixels()));
	CLAMP(handle_pixels, 0, total_pixels);
}

int BC_ScrollBar::cursor_enter_event()
{
	if(top_level->event_win == win)
	{
		if(!highlight_status)
		{
			highlight_status = get_cursor_zone(top_level->cursor_x, 
				top_level->cursor_y);
			draw();
		}
		return 1;
	}
	return 0;
}

void BC_ScrollBar::cursor_leave_event()
{
	if(highlight_status)
	{
		highlight_status = 0;
		draw();
	}
}

int BC_ScrollBar::cursor_motion_event()
{
	if(top_level->event_win == win)
	{
		if(highlight_status && !selection_status)
		{
			int new_highlight_status = 
				get_cursor_zone(top_level->cursor_x, top_level->cursor_y);
			if(new_highlight_status != highlight_status)
			{
				highlight_status = new_highlight_status;
				draw();
			}
		}
		else
		if(selection_status == SCROLL_HANDLE)
		{
			double total_pixels = pixels - get_arrow_pixels() * 2;
			int cursor_pixel = (orientation == SCROLL_HORIZ) ?
				top_level->cursor_x : top_level->cursor_y;
			int new_position = round((double)(cursor_pixel - min_pixel) /
				total_pixels * length);

			if(new_position > length - handlelength) 
				new_position = length - handlelength;
			if(new_position < 0) new_position = 0;

			if(new_position != position)
			{
				position = new_position;
				draw();
				handle_event();
			}
		}
		return 1;
	}
	return 0;
}

int BC_ScrollBar::button_press_event()
{
	if(top_level->event_win == win)
	{
		if(!bound_to)
		{
			top_level->deactivate();
			activate();
		}

		if(get_buttonpress() == 4)
		{
			selection_status = SCROLL_BACKARROW;
			repeat_event(resources.scroll_repeat);
		}
		else
		if(get_buttonpress() == 5)
		{
			selection_status = SCROLL_FWDARROW;
			repeat_count = 0;
			repeat_event(resources.scroll_repeat);
		}
		else
		{
			selection_status = get_cursor_zone(top_level->cursor_x, top_level->cursor_y);
			if(selection_status == SCROLL_HANDLE)
			{
				double total_pixels = pixels - get_arrow_pixels() * 2;
				int cursor_pixel = (orientation == SCROLL_HORIZ) ?
					top_level->cursor_x : top_level->cursor_y;
				min_pixel = cursor_pixel - (int)((double)position / length
					* total_pixels + .5);
				max_pixel = round(cursor_pixel + total_pixels);
				draw();
			}
			else
			if(selection_status)
			{
				top_level->set_repeat(resources.scroll_repeat);
				repeat_count = 0;
				repeat_event(resources.scroll_repeat);
				draw();
			}
		}
		return 1;
	}
	return 0;
}

void BC_ScrollBar::repeat_event(int duration)
{
	if(duration == resources.scroll_repeat &&
		selection_status)
	{
		repeat_count++;
		if(repeat_count == 2) return;
		int new_position = position;
		switch(selection_status)
		{
		case SCROLL_BACKPAGE:
			new_position -= handlelength;
			break;
		case SCROLL_FWDPAGE:
			new_position += handlelength;
			break;
		case SCROLL_BACKARROW:
			new_position -= handlelength / 10;
			break;
		case SCROLL_FWDARROW:
			new_position += handlelength / 10;
			break;
		}

		if(new_position > length - handlelength) new_position = length - handlelength;
		if(new_position < 0) new_position = 0;
		if(new_position != position)
		{
			position = new_position;
			draw();
			handle_event();
		}
	}
}

int BC_ScrollBar::button_release_event()
{
	if(selection_status)
	{
		if(selection_status != SCROLL_HANDLE)
			top_level->unset_repeat(resources.scroll_repeat);

		selection_status = 0;
		draw();
		return 1;
	}
	return 0;
}

int BC_ScrollBar::get_cursor_zone(int cursor_x, int cursor_y)
{
	if(orientation == SCROLL_VERT)
	{
		cursor_x ^= cursor_y;
		cursor_y ^= cursor_x;
		cursor_x ^= cursor_y;
	}

	if(cursor_x >= pixels - get_arrow_pixels())
		return SCROLL_FWDARROW;
	else
	if(cursor_x >= get_arrow_pixels())
	{
		if(cursor_x > handle_pixel + handle_pixels)
			return SCROLL_FWDPAGE;
		else
		if(cursor_x >= handle_pixel)
			return SCROLL_HANDLE;
		else
			return SCROLL_BACKPAGE;
	}
	else
		return SCROLL_BACKARROW;

	return 0;
}

void BC_ScrollBar::activate()
{
	top_level->active_subwindow = this;
}

int BC_ScrollBar::get_value()
{
	return position;
}

int BC_ScrollBar::get_position()
{
	return position;
}

int BC_ScrollBar::get_length()
{
	return length;
}

int BC_ScrollBar::get_pixels()
{
	return pixels;
}

int BC_ScrollBar::in_use()
{
	return selection_status != 0;
}

int BC_ScrollBar::get_handlelength()
{
	return handlelength;
}

void BC_ScrollBar::update_value(int value)
{
	this->position = value;
	draw();
}

void BC_ScrollBar::update_length(int length, int position, int handlelength)
{
	this->length = length;
	this->position = position;
	this->handlelength = handlelength;
	draw();
}

void BC_ScrollBar::reposition_window(int x, int y, int pixels)
{
	if(x != get_x() || y != get_y() || pixels != this->pixels)
	{
		this->pixels = pixels;
		int new_w, new_h;
		calculate_dimensions(new_w, new_h);
		BC_WindowBase::reposition_window(x, y, new_w, new_h);
	}
	draw();
}

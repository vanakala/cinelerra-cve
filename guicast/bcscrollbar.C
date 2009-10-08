
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
#include "bcscrollbar.h"
#include "clip.h"
#include "colors.h"
#include "vframe.h"

#include <string.h>

BC_ScrollBar::BC_ScrollBar(int x, 
	int y, 
	int orientation, 
	int pixels, 
	int64_t length, 
	int64_t position, 
	int64_t handlelength,
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
		this->data = BC_WindowBase::get_resources()->hscroll_data;
	else
		this->data = BC_WindowBase::get_resources()->vscroll_data;

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

int BC_ScrollBar::initialize()
{
//printf("BC_ScrollBar::initialize 1\n");
	set_images(data);
//printf("BC_ScrollBar::initialize 1\n");

	BC_SubWindow::initialize();
//printf("BC_ScrollBar::initialize 1\n");
	draw();
	return 0;
}

void BC_ScrollBar::set_images(VFrame **data)
{
	for(int i = 0; i < SCROLL_IMAGES; i++)
	{
		if(images[i]) delete images[i];
		images[i] = new BC_Pixmap(parent_window, data[i], PIXMAP_ALPHA);
//printf("BC_ScrollBar::set_images %d %p\n", i, data[i]);
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
			return BC_WindowBase::get_resources()->hscroll_data[SCROLL_HANDLE_UP]->get_h();
			break;

		case SCROLL_VERT:
			return BC_WindowBase::get_resources()->vscroll_data[SCROLL_HANDLE_UP]->get_w();
			break;
	}
	return 0;
}

int BC_ScrollBar::get_span()
{
	switch(orientation)
	{
		case SCROLL_HORIZ:
			return data[SCROLL_HANDLE_UP]->get_h();
			break;

		case SCROLL_VERT:
			return data[SCROLL_HANDLE_UP]->get_w();
			break;
	}
	return 0;
}

int BC_ScrollBar::get_arrow_pixels()
{
	switch(orientation)
	{
		case SCROLL_HORIZ:
			return data[SCROLL_BACKARROW_UP]->get_w();
			break;

		case SCROLL_VERT:
			return data[SCROLL_BACKARROW_UP]->get_h();
			break;
	}
	return 0;
}


void BC_ScrollBar::draw()
{
	draw_top_background(parent_window, 0, 0, w, h);
	get_handle_dimensions();

	switch(orientation)
	{
		case SCROLL_HORIZ:


//printf("BC_ScrollBar::draw 1 %d %d\n", selection_status, highlight_status == SCROLL_BACKARROW);
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
//printf("BC_ScrollBar::draw 2 %p\n", images[SCROLL_BACKARROW_HI]);
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
//printf("BC_ScrollBar::draw 2\n");






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





//printf("BC_ScrollBar::draw 2\n");

// handle background
				draw_3segmenth(get_arrow_pixels(),
					0,
					handle_pixel - get_arrow_pixels(),
					images[SCROLL_HANDLE_BG]);

// handle foreground
//printf("BC_ScrollBar::draw 2 %d %d\n", handle_pixel, handle_pixels);
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
//printf("BC_ScrollBar::draw 2\n");

// handle background
				draw_3segmenth(handle_pixel + handle_pixels,
					0,
					get_w() - get_arrow_pixels() - handle_pixel - handle_pixels,
					images[SCROLL_HANDLE_BG]);
//printf("BC_ScrollBar::draw 3 %d %d\n", handle_pixel, handle_pixels);
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
//printf("BC_ScrollBar::draw 2 %p\n", images[SCROLL_BACKARROW_HI]);
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
//printf("BC_ScrollBar::draw 2\n");






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





//printf("BC_ScrollBar::draw 2\n");

// handle background
				draw_3segmentv(0,
					get_arrow_pixels(),
					handle_pixel - get_arrow_pixels(),
					images[SCROLL_HANDLE_BG]);

// handle foreground
//printf("BC_ScrollBar::draw 2 %d %d\n", handle_pixel, handle_pixels);
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
//printf("BC_ScrollBar::draw 2\n");

// handle background
				draw_3segmentv(0,
					handle_pixel + handle_pixels,
					get_h() - get_arrow_pixels() - handle_pixel - handle_pixels,
					images[SCROLL_HANDLE_BG]);
//printf("BC_ScrollBar::draw 3 %d %d\n", handle_pixel, handle_pixels);
			}
			break;
	}
	flash();
}

void BC_ScrollBar::get_handle_dimensions()
{
	int total_pixels = pixels - 
		get_arrow_pixels() * 2;

	if(length > 0)
	{
		handle_pixels = (int64_t)((double)handlelength / 
			length * 
			total_pixels + 
			.5);

		if(handle_pixels < get_resources()->scroll_minhandle)
			handle_pixels = get_resources()->scroll_minhandle;


		handle_pixel = (int64_t)((double)position / 
				length * 
				total_pixels + .5) + 
			get_arrow_pixels();

// Handle pixels is beyond minimum right position.  Clamp it.
		if(handle_pixel > pixels - get_arrow_pixels() - get_resources()->scroll_minhandle)
		{
			handle_pixel = pixels - get_arrow_pixels() - get_resources()->scroll_minhandle;
			handle_pixels = get_resources()->scroll_minhandle;
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
		if(handle_pixels < get_resources()->scroll_minhandle) handle_pixels = get_resources()->scroll_minhandle;
	}
	else
	{
		handle_pixels = total_pixels;
		handle_pixel = get_arrow_pixels();
	}

	CLAMP(handle_pixel, get_arrow_pixels(), (int)(pixels - get_arrow_pixels()));
	CLAMP(handle_pixels, 0, total_pixels);

// printf("BC_ScrollBar::get_handle_dimensions %d %d %d\n", 
// total_pixels, 
// handle_pixel,
// handle_pixels);
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

int BC_ScrollBar::cursor_leave_event()
{
	if(highlight_status)
	{
		highlight_status = 0;
		draw();
	}
	return 0;
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
//printf("BC_ScrollBar::cursor_motion_event 1\n");
			double total_pixels = pixels - get_arrow_pixels() * 2;
			int64_t cursor_pixel = (orientation == SCROLL_HORIZ) ? 
				top_level->cursor_x : 
				top_level->cursor_y;
			int64_t new_position = (int64_t)((double)(cursor_pixel - min_pixel) / 
				total_pixels * length);
//printf("BC_ScrollBar::cursor_motion_event 2\n");

			if(new_position > length - handlelength) 
				new_position = length - handlelength;
			if(new_position < 0) new_position = 0;

			if(new_position != position)
			{
//printf("BC_ScrollBar::cursor_motion_event 3\n");
				position = new_position;
				draw();
				handle_event();
//printf("BC_ScrollBar::cursor_motion_event 4\n");
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
			repeat_event(top_level->get_resources()->scroll_repeat);
		}
		else
		if(get_buttonpress() == 5)
		{
			selection_status = SCROLL_FWDARROW;
			repeat_count = 0;
			repeat_event(top_level->get_resources()->scroll_repeat);
		}
		else
		{
			selection_status = get_cursor_zone(top_level->cursor_x, top_level->cursor_y);
			if(selection_status == SCROLL_HANDLE)
			{
				double total_pixels = pixels - get_arrow_pixels() * 2;
				int64_t cursor_pixel = (orientation == SCROLL_HORIZ) ? top_level->cursor_x : top_level->cursor_y;
				min_pixel = cursor_pixel - (int64_t)((double)position / length * total_pixels + .5);
				max_pixel = (int)(cursor_pixel + total_pixels);
				draw();
			}
			else
			if(selection_status)
			{
				top_level->set_repeat(top_level->get_resources()->scroll_repeat);
				repeat_count = 0;
				repeat_event(top_level->get_resources()->scroll_repeat);
				draw();
			}
		}
		return 1;
	}
	return 0;
}

int BC_ScrollBar::repeat_event(int64_t duration)
{
	if(duration == top_level->get_resources()->scroll_repeat && 
		selection_status)
	{
		repeat_count++;
		if(repeat_count == 2) return 0;
		int64_t new_position = position;
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
		return 1;
	}
	return 0;
}

int BC_ScrollBar::button_release_event()
{
//printf("BC_ScrollBar::button_release_event %d\n", selection_status);
	if(selection_status)
	{
		if(selection_status != SCROLL_HANDLE)
			top_level->unset_repeat(top_level->get_resources()->scroll_repeat);

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

int BC_ScrollBar::activate()
{
	top_level->active_subwindow = this;
//printf("BC_ScrollBar::activate %p %p\n", top_level->active_subwindow, this);
	return 0;
}

int64_t BC_ScrollBar::get_value()
{
	return position;
}

int64_t BC_ScrollBar::get_position()
{
	return position;
}

int64_t BC_ScrollBar::get_length()
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

int64_t BC_ScrollBar::get_handlelength()
{
	return handlelength;
}

int BC_ScrollBar::update_value(int64_t value)
{
	this->position = value;
	draw();
	return 0;
}

int BC_ScrollBar::update_length(int64_t length, int64_t position, int64_t handlelength)
{
	this->length = length;
	this->position = position;
	this->handlelength = handlelength;
	draw();
	return 0;
}

int BC_ScrollBar::reposition_window(int x, int y, int pixels)
{
	if(x != get_x() || y != get_y() || pixels != this->pixels)
	{
		this->pixels = pixels;
		int new_w, new_h;
		calculate_dimensions(new_w, new_h);
		BC_WindowBase::reposition_window(x, y, new_w, new_h);
	}
	draw();
	return 0;
}


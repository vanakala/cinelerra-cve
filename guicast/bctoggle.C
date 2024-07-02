// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcpixmap.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bctoggle.h"
#include "clip.h"
#include "colors.h"
#include "cursors.h"
#include "fonts.h"
#include "vframe.h"

#include <string.h>

BC_Toggle::BC_Toggle(int x, int y, VFrame **data, int value, const char *caption,
	int bottom_justify, int font, int color)
 : BC_SubWindow(x, y, 0, 0, -1)
{
	this->data = data;
	for(int i = 0; i < 5; i++)
		images[i] = 0;
	bg_image = 0;
	status = value ? BC_Toggle::TOGGLE_CHECKED : BC_Toggle::TOGGLE_UP;
	this->value = value;
	this->caption = 0;
	if(caption && *caption)
		this->caption = strdup(caption);
	this->bottom_justify = bottom_justify;
	this->font = font;
	if(color >= 0)
		this->color = color;
	else
		this->color = resources.default_text_color;
	select_drag = 0;
	enabled = 1;
	underline = -1;
	is_radial = 0;
}

BC_Toggle::~BC_Toggle()
{
	free(caption);
	for(int i = 0; i < 5; i++)
		delete images[i];
	delete bg_image;
}

void BC_Toggle::initialize()
{
// Get the image
	set_images(data);
	calculate_extents(this, data, bottom_justify, &text_line, &w, &h,
		&toggle_x, &toggle_y, &text_x, &text_y, &text_w, &text_h, caption);

// Create the subwindow
	BC_SubWindow::initialize();
	set_cursor(UPRIGHT_ARROW_CURSOR);
// Display the bitmap
	draw_face();
}

void BC_Toggle::calculate_extents(BC_WindowBase *gui, VFrame **images,
	int bottom_justify, int *text_line, int *w, int *h,
	int *toggle_x, int *toggle_y, int *text_x, int *text_y,
	int *text_w, int *text_h, const char *caption)
{
	VFrame *frame = images[0];

	*w = frame->get_w();
	*h = frame->get_h();
	*toggle_x = 0;
	*toggle_y = 0;
	*text_x = *w + 5;
	*text_y = 0;
	*text_w = 0;
	*text_h = 0;

	if(caption)
	{
		*text_w = gui->get_text_width(MEDIUMFONT, caption);
		*text_h = gui->get_text_height(MEDIUMFONT);

		if(resources.toggle_highlight_bg)
		{
			*text_w += resources.toggle_text_margin * 2;
			*text_h = MAX(*text_h, resources.toggle_highlight_bg->get_h());
		}

		if(*text_h > *h)
		{
			*toggle_y = (*text_h - *h) >> 1;
			*h = *text_h;
		}
		else
			*text_y = (*h - *text_h) >> 1;

		if(bottom_justify)
		{
			*text_y = *h - *text_h;
			*text_line = *h - gui->get_text_descent(MEDIUMFONT);
		}
		else
			*text_line = *text_y + gui->get_text_ascent(MEDIUMFONT);

		*w = *text_x + *text_w;
	}
}


void BC_Toggle::set_images(VFrame **data)
{
	delete bg_image;

	bg_image = 0;

	for(int i = 0; i < 5; i++)
	{
		delete images[i];
		images[i] = new BC_Pixmap(top_level, data[i], PIXMAP_ALPHA);
	}

	if(has_caption())
	{
		if(resources.toggle_highlight_bg)
		{
			bg_image = new BC_Pixmap(top_level,
				resources.toggle_highlight_bg,
				PIXMAP_ALPHA);
		}
	}
}

void BC_Toggle::set_underline(int number)
{
	this->underline = number;
}

void BC_Toggle::set_select_drag(int value)
{
	this->select_drag = value;
}

void BC_Toggle::draw_face()
{
	top_level->lock_window("BC_Toggle::draw_face");
	draw_top_background(parent_window, 0, 0, get_w(), get_h());
	if(has_caption())
	{
		if(enabled && (status == BC_Toggle::TOGGLE_UPHI ||
			status == BC_Toggle::TOGGLE_DOWN ||
			status == BC_Toggle::TOGGLE_CHECKEDHI))
		{
// Draw highlight image
			if(bg_image)
			{
				int x = text_x;
				int y = text_line - get_text_ascent(MEDIUMFONT) / 2 -
						bg_image->get_h() / 2;
				int w = text_w;

				draw_3segmenth(x, y, w, bg_image);
			}
			else
// Draw a plain box
			{
				set_color(LTGREY);
				draw_box(text_x,
					text_line - get_text_ascent(MEDIUMFONT),
					get_w() - text_x,
					get_text_height(MEDIUMFONT));
			}
		}

		set_opaque();
		if(enabled)
			set_color(color);
		else
			set_color(MEGREY);
		set_font(font);
		draw_text(text_x + resources.toggle_text_margin,
			text_line, caption);

// Draw underline
		if(underline >= 0)
		{
			int x = text_x + resources.toggle_text_margin;
			int y = text_line + 1;
			int x1 = get_text_width(current_font, caption, underline) + x;
			int x2 = get_text_width(current_font, caption, underline + 1) + x;

			draw_line(x1, y, x2, y);
			draw_line(x1, y + 1, (x2 + x1) / 2, y + 1);
		}
	}

	draw_pixmap(images[status]);
	flash();
	top_level->unlock_window();
}

void BC_Toggle::enable()
{
	enabled = 1;
	if(parent_window)
		draw_face();
}

void BC_Toggle::disable()
{
	enabled = 0;
	if(parent_window)
		draw_face();
}

void BC_Toggle::set_status(int value)
{
	this->status = value;
}

void BC_Toggle::repeat_event(int duration)
{
	if(duration == resources.tooltip_delay && tooltip_wtext &&
		(status == BC_Toggle::TOGGLE_UPHI ||
		status == BC_Toggle::TOGGLE_CHECKEDHI) && !tooltip_done)
	{
		show_tooltip();
		tooltip_done = 1;
	}
}

int BC_Toggle::cursor_enter_event()
{
	if(top_level->event_win == win && enabled)
	{
		tooltip_done = 0;
		if(top_level->button_down)
			status = BC_Toggle::TOGGLE_DOWN;
		else
			status = value ? BC_Toggle::TOGGLE_CHECKEDHI :
				BC_Toggle::TOGGLE_UPHI;
		draw_face();
	}
	return 0;
}

void BC_Toggle::cursor_leave_event()
{
	hide_tooltip();
	if(!value && status == BC_Toggle::TOGGLE_UPHI)
	{
		status = BC_Toggle::TOGGLE_UP;
		draw_face();
	}
	else if(status == BC_Toggle::TOGGLE_CHECKEDHI)
	{
		status = BC_Toggle::TOGGLE_CHECKED;
		draw_face();
	}
}

int BC_Toggle::button_press_event()
{
	hide_tooltip();
	if(top_level->event_win == win && get_buttonpress() == 1 && enabled)
	{
		status = BC_Toggle::TOGGLE_DOWN;

// Change value now for select drag mode.
// Radial always goes to 1
		if(select_drag)
		{
			if(!is_radial)
				value = !value;
			else
				value = 1;
			top_level->toggle_drag = 1;
			top_level->toggle_value = value;
			handle_event();
		}

		draw_face();
		return 1;
	}
	return 0;
}

int BC_Toggle::button_release_event()
{
	int result = 0;
	hide_tooltip();

	if(top_level->event_win == win)
	{
// Keep value regardless of status if drag mode.
		if(select_drag)
		{
			if(value)
				status = BC_Toggle::TOGGLE_CHECKEDHI;
			else
				status = BC_Toggle::TOGGLE_UPHI;
			top_level->toggle_drag = 0;
		}
		else
// Change value only if button down for default mode.
		if(status == BC_Toggle::TOGGLE_DOWN)
		{
// Radial always goes to 1.
			if(!value || is_radial)
			{
				status = BC_Toggle::TOGGLE_CHECKEDHI;
				value = 1;
			}
			else
			{
				status = BC_Toggle::TOGGLE_UPHI;
				value = 0;
			}
			result = handle_event();
		}
		draw_face();
		return result;
	}
	return 0;
}

int BC_Toggle::cursor_motion_event()
{
	if(top_level->button_down && 
		top_level->event_win == win && 
		!cursor_inside())
	{
		if(status == BC_Toggle::TOGGLE_DOWN)
		{
			if(value)
				status = BC_Toggle::TOGGLE_CHECKED;
			else
				status = BC_Toggle::TOGGLE_UP;
			draw_face();
		}
		else if(status == BC_Toggle::TOGGLE_UPHI)
		{
			status = BC_Toggle::TOGGLE_CHECKEDHI;
			draw_face();
		}
	}
	return 0;
}

int BC_Toggle::get_value()
{
	return value;
}

void BC_Toggle::set_value(int value, int draw)
{
	if(value != this->value)
	{
		this->value = value;
		if(value)
		{
			switch(status)
			{
			case BC_Toggle::TOGGLE_UP:
				status = BC_Toggle::TOGGLE_CHECKED;
				break;
			case BC_Toggle::TOGGLE_UPHI:
				status = BC_Toggle::TOGGLE_CHECKEDHI;
				break;
			}
		}
		else
		{
			switch(status)
			{
			case BC_Toggle::TOGGLE_CHECKED:
				status = BC_Toggle::TOGGLE_UP;
				break;
			case BC_Toggle::TOGGLE_CHECKEDHI:
				status = BC_Toggle::TOGGLE_UPHI;
				break;
			}
		}
		if(draw) draw_face();
	}
}

void BC_Toggle::update(int value, int draw)
{
	set_value(value, draw);
}

void BC_Toggle::reposition_window(int x, int y)
{
	BC_WindowBase::reposition_window(x, y);
	draw_face();
}

int BC_Toggle::has_caption()
{
	return(caption && *caption);
}

BC_Radial::BC_Radial(int x, int y, int value, const char *caption,
	int font, int color)
 : BC_Toggle(x, y,BC_WindowBase::get_resources()->radial_images,
	value, caption, 0, font, color)
{
	is_radial = 1;
}

BC_CheckBox::BC_CheckBox(int x, int y, int value,  const char *caption,
	int font, int color)
 : BC_Toggle(x, y, BC_WindowBase::get_resources()->checkbox_images,
	value, caption, 0, font, color)
{
	this->value = 0;
}

BC_CheckBox::BC_CheckBox(int x, int y, int *value, const char *caption,
	int font, int color)
 : BC_Toggle(x, y, BC_WindowBase::get_resources()->checkbox_images,
	*value, caption, 1, font, color)
{
	this->value = value;
}

int BC_CheckBox::handle_event()
{
	*value = get_value();
	return 1;
}


BC_Label::BC_Label(int x, int y, int value, int font, int color)
 : BC_Toggle(x, y, BC_WindowBase::get_resources()->label_images,
	value, "", 0, font, color)
{
}

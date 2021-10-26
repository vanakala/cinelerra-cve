// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcbutton.h"
#include "bcpixmap.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "colors.h"
#include "fonts.h"
#include "keys.h"
#include "language.h"
#include "vframe.h"

#include <string.h>
#include <unistd.h>

#define BUTTON_UP 0
#define BUTTON_UPHI 1
#define BUTTON_DOWNHI 2

BC_Button::BC_Button(int x, 
	int y, 
	VFrame **data)
 : BC_SubWindow(x, y, 0, 0, -1)
{
	this->data = data;
	for(int i = 0; i < 3; i++) images[i] = 0;
	if(!data) printf("BC_Button::BC_Button data == 0\n");
	status = BUTTON_UP;
	this->w_argument = 0;
	underline_number = -1;
	enabled = 1;
}

BC_Button::BC_Button(int x, 
	int y, 
	int w, 
	VFrame **data)
 : BC_SubWindow(x, y, 0, 0, -1)
{
	this->data = data;
	this->w_argument = w;
	for(int i = 0; i < 3; i++) images[i] = 0;
	if(!data) printf("BC_Button::BC_Button data == 0\n");
	status = BUTTON_UP;
	underline_number = -1;
	enabled = 1;
}

BC_Button::~BC_Button()
{
	for(int i = 0; i < 3; i++)
		if(images[i]) delete images[i];
}

void BC_Button::initialize()
{
// Get the image
	set_images(data);

// Create the subwindow
	BC_SubWindow::initialize();

// Display the bitmap
	draw_face();
}

void BC_Button::reposition_window(int x, int y)
{
	BC_WindowBase::reposition_window(x, y);
	draw_face();
}


void BC_Button::update_bitmaps(VFrame **data)
{
	top_level->lock_window("BC_Button::update_bitmaps");
	this->data = data;
	set_images(data);
	draw_top_background(parent_window, 0, 0, w, h);
	draw_face();
	top_level->unlock_window();
}

void BC_Button::enable()
{
	set_enabled(1);
}

void BC_Button::disable()
{
	set_enabled(0);
}

void BC_Button::set_enabled(int value)
{
	enabled = value;
	draw_face();
}

void BC_Button::set_underline(int number)
{
	this->underline_number = number;
}

void BC_Button::set_images(VFrame **data)
{
	for(int i = 0; i < 3; i++)
	{
		if(images[i]) delete images[i];
		images[i] = new BC_Pixmap(parent_window, data[i], PIXMAP_ALPHA);
	}

	if(w_argument > 0)
		w = w_argument;
	else
		w = images[BUTTON_UP]->get_w();

	h = images[BUTTON_UP]->get_h();
}

void BC_Button::draw_face()
{
	top_level->lock_window("BC_Button::draw_face");
	draw_top_background(parent_window, 0, 0, w, h);
	pixmap->draw_pixmap(images[status], 
			0, 
			0,
			w,
			h,
			0,
			0);
	flash();
	top_level->unlock_window();
}

void BC_Button::repeat_event(int duration)
{
	if(duration == resources.tooltip_delay &&
		tooltip_wtext &&
		status == BUTTON_UPHI &&
		!tooltip_done)
	{
		show_tooltip();
		tooltip_done = 1;
	}
}

int BC_Button::cursor_enter_event()
{
	if(top_level->event_win == win && enabled)
	{
		tooltip_done = 0;
		if(top_level->button_down)
		{
			status = BUTTON_DOWNHI;
		}
		else
		if(status == BUTTON_UP) status = BUTTON_UPHI;
		draw_face();
	}
	return 0;
}

void BC_Button::cursor_leave_event()
{
	if(status == BUTTON_UPHI)
	{
		status = BUTTON_UP;
		draw_face();
		hide_tooltip();
	}
}

int BC_Button::button_press_event()
{
	if(top_level->event_win == win && get_buttonpress() == 1 && enabled)
	{
		hide_tooltip();
		if(status == BUTTON_UPHI || status == BUTTON_UP) status = BUTTON_DOWNHI;
		draw_face();
		return 1;
	}
	return 0;
}

int BC_Button::button_release_event()
{
	if(top_level->event_win == win)
	{
		hide_tooltip();
		if(status == BUTTON_DOWNHI) 
		{
			status = BUTTON_UPHI;
			draw_face();

			if(cursor_inside())
			{
				handle_event();
				return 1;
			}
		}
	}
	return 0;
}

int BC_Button::cursor_motion_event()
{
	if(top_level->button_down && top_level->event_win == win && 
		status == BUTTON_DOWNHI && !cursor_inside())
	{
		status = BUTTON_UP;
		draw_face();
	}
	return 0;
}


BC_OKButton::BC_OKButton(int x, int y)
 : BC_Button(x, y, resources.ok_images)
{
}

BC_OKButton::BC_OKButton(BC_WindowBase *parent_window, VFrame **images)
 : BC_Button(10, 
	parent_window->get_h() - images[0]->get_h() - 10, 
	images)
{
	set_tooltip(_("OK"));
}

BC_OKButton::BC_OKButton(BC_WindowBase *parent_window)
 : BC_Button(10, 
	parent_window->get_h() - resources.ok_images[0]->get_h() - 10,
	resources.ok_images)
{
	set_tooltip(_("OK"));
}

int BC_OKButton::handle_event()
{
	get_top_level()->set_done(0);
	return 0;
}

void BC_OKButton::resize_event(int w, int h)
{
	reposition_window(10,
		h - resources.cancel_images[0]->get_h() - 10);
}

int BC_OKButton::keypress_event()
{
	if(get_keypress() == RETURN) return handle_event();
	return 0;
}

int BC_OKButton::calculate_h()
{
	return resources.ok_images[0]->get_h();
}

int BC_OKButton::calculate_w()
{
	return resources.ok_images[0]->get_w();
}


BC_CancelButton::BC_CancelButton(int x, int y)
 : BC_Button(x, y, resources.cancel_images)
{
	set_tooltip(_("Cancel"));
}

BC_CancelButton::BC_CancelButton(BC_WindowBase *parent_window)
 : BC_Button(parent_window->get_w() - resources.cancel_images[0]->get_w() - 10,
	parent_window->get_h() - resources.cancel_images[0]->get_h() - 10,
	resources.cancel_images)
{
	set_tooltip(_("Cancel"));
}

BC_CancelButton::BC_CancelButton(BC_WindowBase *parent_window, VFrame **images)
 : BC_Button(parent_window->get_w() - images[0]->get_w() - 10, 
	parent_window->get_h() - images[0]->get_h() - 10, 
	images)
{
	set_tooltip(_("Cancel"));
}

int BC_CancelButton::handle_event()
{
	get_top_level()->set_done(1);
	return 1;
}

void BC_CancelButton::resize_event(int w,int h)
{
	reposition_window(w - resources.cancel_images[0]->get_w() - 10,
		h - resources.cancel_images[0]->get_h() - 10);
}

int BC_CancelButton::keypress_event()
{
	if(get_keypress() == ESC) return handle_event();
	return 0;
}

int BC_CancelButton::calculate_h()
{
	return resources.cancel_images[0]->get_h();
}

int BC_CancelButton::calculate_w()
{
	return resources.cancel_images[0]->get_w();
}

#define LEFT_DN  0
#define LEFT_HI  1
#define LEFT_UP  2
#define MID_DN   3
#define MID_HI   4
#define MID_UP   5
#define RIGHT_DN 6
#define RIGHT_HI 7
#define RIGHT_UP 8

BC_GenericButton::BC_GenericButton(int x, int y, const char *text, VFrame **data)
 : BC_Button(x, y,
	data ? data : resources.generic_button_images)
{
	strcpy(this->text, text);
}

BC_GenericButton::BC_GenericButton(int x, int y, int w, const char *text, VFrame **data)
 : BC_Button(x, y, w,
	data ? data : resources.generic_button_images)
{
	strcpy(this->text, text);
}

void BC_GenericButton::set_images(VFrame **data)
{
	for(int i = 0; i < 3; i++)
	{
		if(images[i]) delete images[i];
		images[i] = new BC_Pixmap(parent_window, data[i], PIXMAP_ALPHA);
	}

	if(w_argument)
		w = w_argument;
	else
		w = get_text_width(MEDIUMFONT, text) + 
				resources.generic_button_margin * 2;

	h = images[BUTTON_UP]->get_h();
}

int BC_GenericButton::calculate_w(BC_WindowBase *gui, const char *text)
{
	return gui->get_text_width(MEDIUMFONT, text) +
		resources.generic_button_margin * 2;
}

int BC_GenericButton::calculate_h()
{
	return resources.generic_button_images[0]->get_h();
}

void BC_GenericButton::draw_face()
{
	top_level->lock_window("BC_GenericButton::draw_face");
	draw_top_background(parent_window, 0, 0, get_w(), get_h());
	draw_3segmenth(0, 0, get_w(), images[status]);

	if(enabled)
		set_color(resources.default_text_color);
	else
		set_color(resources.disabled_text_color);
	set_font(MEDIUMFONT);

	int x, y, w;

	y = (int)((float)get_h() / 2 + get_text_ascent(MEDIUMFONT) / 2 - 2);
	w = get_text_width(current_font, text, strlen(text)) + 
		resources.generic_button_margin * 2;
	x = get_w() / 2 - w / 2 + resources.generic_button_margin;
	if(status == BUTTON_DOWNHI)
	{
		x++;
		y++;
	}
	draw_text(x, 
		y, 
		text);

	if(underline_number >= 0)
	{
		y++;
		int x1 = get_text_width(current_font, text, underline_number) + 
			x + resources.toggle_text_margin;
		int x2 = get_text_width(current_font, text, underline_number + 1) + 
			x + resources.toggle_text_margin;
		draw_line(x1, y, x2, y);
		draw_line(x1, y + 1, (x2 + x1) / 2, y + 1);
	}

	flash();
	top_level->unlock_window();
}


BC_OKTextButton::BC_OKTextButton(BC_WindowBase *parent_window)
 : BC_GenericButton(10,
	parent_window->get_h() - BC_GenericButton::calculate_h() - 10,
	_("OK"))
{
	this->parent_window = parent_window;
}

void BC_OKTextButton::resize_event(int w, int h)
{
	reposition_window(10,
		parent_window->get_h() - BC_GenericButton::calculate_h() - 10);
}

int BC_OKTextButton::handle_event()
{
	get_top_level()->set_done(0);
	return 0;
}

int BC_OKTextButton::keypress_event()
{
	if(get_keypress() == RETURN) return handle_event();
	return 0;
}


BC_CancelTextButton::BC_CancelTextButton(BC_WindowBase *parent_window)
 : BC_GenericButton(parent_window->get_w() - BC_GenericButton::calculate_w(parent_window, _("Cancel")) - 10,
	parent_window->get_h() - BC_GenericButton::calculate_h() - 10,
	_("Cancel"))
{
	this->parent_window = parent_window;
}

void BC_CancelTextButton::resize_event(int w, int h)
{
	reposition_window(parent_window->get_w() - BC_GenericButton::calculate_w(parent_window, _("Cancel")) - 10,
		parent_window->get_h() - BC_GenericButton::calculate_h() - 10);
}

int BC_CancelTextButton::handle_event()
{
	get_top_level()->set_done(1);
	return 1;
}

int BC_CancelTextButton::keypress_event()
{
	if(get_keypress() == ESC) return handle_event();
	return 0;
}

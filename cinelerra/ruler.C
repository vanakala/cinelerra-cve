// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2013 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "bcsignals.h"
#include "bcresources.h"
#include "cursors.h"
#include "language.h"
#include "mainsession.h"
#include "mainmenu.h"
#include "mwindow.h"
#include "ruler.h"
#include "thread.h"

Ruler::Ruler()
 : Thread()
{
	gui = new RulerGUI(this);
}

Ruler::~Ruler()
{
	if(gui)
	{
		gui->set_done(0);
		join();
	}
	delete gui;
}

void Ruler::run()
{
	gui->run_window();
}

RulerGUI::RulerGUI(Ruler *thread)
 : BC_Window(MWindow::create_title(N_("Ruler")),
	mainsession->ruler_x,
	mainsession->ruler_y,
	10,
	10,
	0, // minw
	0, // minh
	0, // allow resize
	0, // Private color
	1, // hide
	BC_WindowBase::get_resources()->bg_color, // bg_color
	0, // display name
	0, // group_it
	WINDOW_SPLASH) // splash
{
	set_icon(mwindow_global->get_window_icon());
	width = 0;
	height = 0;
	resize = 0;
	separate_cursor = 0;
	// Limit ruler length and position
	root_w = get_root_w(0, 0);
	root_h = get_root_h(0);
	max_length = root_h * 4 / 5;
	min_length = RULER_MIN_LENGTH;
	if(mainsession->ruler_length < min_length)
		mainsession->ruler_length;
	if(mainsession->ruler_length > max_length)
		mainsession->ruler_length = max_length;
	draw_ruler();
}

int RulerGUI::ticklength(int tick)
{
	int h = 5;

	if(tick % 10 == 0)
	{
		if(tick % 50)
			h = 9;
		else
			h = 12;
	}
	return h;
}

void RulerGUI::draw_ruler()
{
	char string[32];
	int th, tw, tb;
	int h, w;
	int r_width;

	th = get_text_height(SMALLFONT) - 2;
	tw = get_text_width(SMALLFONT, "000");
	set_font(SMALLFONT);
	if(mainsession->ruler_orientation == RULER_HORIZ)
	{
		r_width = 20 + th;
		tb = 14 + th;
		if(get_h() != height || get_w() != width)
		{
			resize_window(mainsession->ruler_length, r_width);
			BC_WindowBase::resize_event(mainsession->ruler_length, r_width);
		}
		// draw bg
		clear_box(0, 0, mainsession->ruler_length, r_width);
		set_color(BC_WindowBase::get_resources()->default_text_color);
		draw_rectangle(0, 0, mainsession->ruler_length, r_width);
		for(int i = 5; i <  mainsession->ruler_length; i += 5)
		{
			h = ticklength(i);
			draw_line(i, 0, i, h);
		}
		for(int i = 50; i < mainsession->ruler_length; i += 50)
		{
			sprintf(string, "%d", i);
			w = get_text_width(SMALLFONT, string);
			draw_text(i - w / 2, tb, string);
		}
		draw_text(10, tb, "x");
		x_cl = 10 + tw / 6;
		y_cl = tb - th / 3;
		width = mainsession->ruler_length;
		height = r_width;
	}
	else
	{
		r_width = 18 + tw;
		if(get_h() != height || get_w() != width)
		{
			resize_window(r_width, mainsession->ruler_length);
			BC_WindowBase::resize_event(r_width, mainsession->ruler_length);
		}
		// draw bg
		clear_box(0, 0, r_width, mainsession->ruler_length);
		set_color(BC_WindowBase::get_resources()->default_text_color);
		draw_rectangle(0, 0, r_width, mainsession->ruler_length);
		for(int i = 5; i < mainsession->ruler_length; i += 5)
		{
			w = ticklength(i);
			draw_line(0, i, w, i);
		}
		for(int i = 50; i < mainsession->ruler_length; i += 50)
		{
			sprintf(string, "%d", i);
			draw_text(14, i + th / 2, string);
		}
		draw_text(18, 15, "x");
		x_cl = 18 + tw / 6;
		y_cl = 15 - th / 3;
		height = mainsession->ruler_length;
		width = r_width;
	}
	flash();
}

void RulerGUI::resize_event(int w, int h)
{
	mainsession->ruler_x = get_x();
	mainsession->ruler_y = get_y();
	if(mainsession->ruler_orientation == RULER_HORIZ)
		mainsession->ruler_length = w;
	else
		mainsession->ruler_length = h;
	if(mainsession->ruler_length < min_length)
		mainsession->ruler_length = min_length;
	if(mainsession->ruler_length > max_length)
		mainsession->ruler_length = max_length;
	draw_ruler();
}

void RulerGUI::translation_event()
{
	mainsession->ruler_x = get_x();
	mainsession->ruler_y = get_y();
}

void RulerGUI::close_event()
{
	hide_window();
	mwindow_global->mark_ruler_hidden();
}

int RulerGUI::keypress_event()
{
	if(get_keypress() == 'w' || get_keypress() == 'W')
	{
		close_event();
		return 1;
	}
	return 0;
}

int RulerGUI::cursor_motion_event()
{
	int cpos;

	if(get_button_down())
		return 0;

	if(mainsession->ruler_orientation == RULER_HORIZ)
		cpos = get_cursor_x();
	else
		cpos = get_cursor_y();
	if(cpos > mainsession->ruler_length - 5)
	{
		if(!separate_cursor)
		{
			set_cursor(mainsession->ruler_orientation == RULER_HORIZ ?
				HSEPARATE_CURSOR : VSEPARATE_CURSOR);
			separate_cursor = 1;
		}
	}
	else if(separate_cursor)
	{
		set_cursor(ARROW_CURSOR);
		separate_cursor = 0;
	}
	return 0;
}

int RulerGUI::button_release_event()
{
	int button = get_buttonpress();
	int x = get_cursor_x();
	int y = get_cursor_y();

	if(button == 3)
	{
		mainsession->ruler_orientation =
			mainsession->ruler_orientation == RULER_HORIZ ? RULER_VERT : RULER_HORIZ;
		width = height = 0;
		draw_ruler();
		return 1;
	}
	if(button == 2)
	{
		close_event();
		return 1;
	}
	if(button == 1 && x > x_cl - 5 && x < x_cl + 5 && y > y_cl - 5 && y < y_cl + 5)
	{
		close_event();
		return 1;
	}
	return 0;
}

int RulerGUI::drag_start_event()
{
	int cpos;
	if(mainsession->ruler_orientation == RULER_HORIZ)
		cpos = get_cursor_x();
	else
		cpos = get_cursor_y();
	if(cpos > mainsession->ruler_length - 5)
		resize = 1;
	else
		set_cursor(MOVE_CURSOR);
	return 1;
}

void RulerGUI::drag_motion_event()
{
	if(resize)
	{
		if(mainsession->ruler_orientation == RULER_HORIZ)
			resize_event(get_cursor_x(), height);
		else
			resize_event(width, get_cursor_y());
	}
	else
	{
		int new_x = get_x() - get_drag_x() + get_cursor_x();
		int new_y = get_y() - get_drag_y() + get_cursor_y();

		if(new_x < 0)
			new_x = 0;
		else if(new_x + width >= root_w)
			new_x = root_w - width;
		if(new_y < 0)
			new_y = 0;
		else if(new_y + height >= root_h)
			new_y = root_h - height;

		reposition_window(new_x, new_y, width, height);
	}
}

void RulerGUI::drag_stop_event()
{
	resize = 0;
	set_cursor(ARROW_CURSOR);
}

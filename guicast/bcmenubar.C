// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcmenu.h"
#include "bcmenubar.h"
#include "bcmenupopup.h"
#include "bcpixmap.h"
#include "bcpopup.h"
#include "bcpopupmenu.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "colors.h"
#include "fonts.h"
#include "vframe.h"

#include <string.h>

// ================================== Menu bar ==================================

BC_MenuBar::BC_MenuBar(int x, int y, int w)
 : BC_SubWindow(x, y, w, 0, -1)
{
// Height is really determined by the font in create_tool_objects.
	button_releases = 0;
	active = 0;
	menu_bar_bg = 0;
	for(int i = 0; i < 3; i++)
		menu_title_bg[i] = 0;
}


BC_MenuBar::~BC_MenuBar()
{
// Delete all titles.
	for(int i = 0; i < menu_titles.total; i++) delete menu_titles.values[i];
	menu_titles.remove_all();
	delete menu_bar_bg;
	for(int i = 0; i < 3; i++)
		delete menu_title_bg[i];
}

void BC_MenuBar::initialize()
{
	int x, w;
// Initialize dimensions
	h = calculate_height(this);
	bg_color = resources.menu_up;

	if(resources.menu_bar_bg)
		menu_bar_bg = new BC_Pixmap(this,
			resources.menu_bar_bg);

	if(resources.menu_title_bg)
	{
		for(int i = 0; i < 3; i++)
			menu_title_bg[i] = new BC_Pixmap(this,
				resources.menu_title_bg[i]);
	}

// Create the subwindow
	BC_SubWindow::initialize();

	if(resources.menu_bg)
		set_background(resources.menu_bg);
	draw_face();

	for(int i = 0; i < menu_titles.total; i++)
	{
		if(i == 0)
			x = 2;
		else
			x = menu_titles.values[i - 1]->x +
				menu_titles.values[i - 1]->w;

		w = get_text_width(MEDIUMFONT, menu_titles.values[i]->text) + 20;
// initialize and draw
		menu_titles.values[i]->initialize(top_level, this, x, 2, w, get_h() - 4);
	}
}

int BC_MenuBar::calculate_height(BC_WindowBase *window)
{
	if(resources.menu_bar_bg)
		return resources.menu_bar_bg->get_h();
	else
		return window->get_text_height(MEDIUMFONT) + 8;
}

void BC_MenuBar::draw_items()
{
	for(int i = 0; i < menu_titles.total; i++)
		menu_titles.values[i]->draw_items();
	flush();
}

void BC_MenuBar::add_menu(BC_Menu* menu)
{
	int x, w;

	menu_titles.append(menu);

	if(top_level)
	{
		if(menu_titles.total <= 1)
			x = 2;
		else
			x = menu_titles.values[menu_titles.total - 2]->x +
				menu_titles.values[menu_titles.total - 2]->w;

		w = get_text_width(MEDIUMFONT, menu->text) + 20;
// initialize and draw
		menu->initialize(top_level, this, x, 2, w, get_h() - 4);
	}
}

void BC_MenuBar::focus_out_event()
{
	deactivate();
}

int BC_MenuBar::button_press_event()
{
	int result = 0;

	for(int i = 0; i < menu_titles.total && !result; i++)
	{
		result = menu_titles.values[i]->dispatch_button_press();
	}

	return result;
}

int BC_MenuBar::button_release_event()
{
	int result = 0;

	button_down = 0;
	button_releases++;
	for(int i = 0; i < menu_titles.total; i++)
	{
		result += menu_titles.values[i]->dispatch_button_release();
	}

// Button was released outside any menu.
	if(!result)
	{
		deactivate();
	}
	return result;
}

void BC_MenuBar::resize_event(int w, int h)
{
	resize_window(w, get_h());
	draw_face();
	for(int i = 0; i < menu_titles.total; i++)
	{
		menu_titles.values[i]->draw_title();
	}
}

int BC_MenuBar::keypress_event()
{
	int result = 0;
	if(!top_level->active_subwindow || !top_level->active_subwindow->uses_text())
	{
		for(int i = 0; i < menu_titles.total && !result; i++)
		{
			result = menu_titles.values[i]->dispatch_keypress();
		}
	}
	return result;
}

int BC_MenuBar::cursor_motion_event()
{
	int result = 0;
	for(int i = 0; i < menu_titles.total && !result; i++)
	{
		result = menu_titles.values[i]->dispatch_motion_event();
	}
	return result;
}

void BC_MenuBar::cursor_leave_event()
{
	for(int i = 0; i < menu_titles.total; i++)
	{
		menu_titles.values[i]->dispatch_cursor_leave();
	}
}

int BC_MenuBar::cursor_enter_event()
{
	if(active) return 1;
	return 0;
}

void BC_MenuBar::translation_event()
{
	if(active)
	{
		for(int i = 0; i < menu_titles.total; i++)
		{
			menu_titles.values[i]->dispatch_translation_event();
		}
	}
}

void BC_MenuBar::activate()
{
	top_level->deactivate();
	top_level->active_menubar = this;
	active = 1;
}

void BC_MenuBar::deactivate()
{
	for(int i = 0; i < menu_titles.total; i++)
	{
		menu_titles.values[i]->deactivate_menu();
	}
	top_level->active_menubar = 0;
	active = 0;
}

void BC_MenuBar::unhighlight()
{
	for(int i = 0; i < menu_titles.total; i++)
	{
		menu_titles.values[i]->unhighlight();
	}
}

void BC_MenuBar::draw_face()
{
	top_level->lock_window("BC_MenuBar::draw_face");
	if(menu_bar_bg)
	{
		draw_9segment(0, 0, get_w(), get_h(), menu_bar_bg);
	}
	else
	{
		int lx,ly,ux,uy;
		int h, w;

		h = get_h();
		w = get_w();
		h--; 
		w--;

		lx = 1;  ly = 1;
		ux = w - 1;  uy = h - 1;
		set_color(resources.menu_light);
		draw_line(0, 0, 0, uy);
		draw_line(0, 0, ux, 0);

		set_color(resources.menu_shadow);
		draw_line(ux, ly, ux, uy);
		draw_line(lx, uy, ux, uy);
		set_color(BLACK);
		draw_line(w, 0, w, h);
		draw_line(0, h, w, h);
	}
	flash();
	top_level->unlock_window();
}

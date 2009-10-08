
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
#include <string.h>
#include "vframe.h"


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

int BC_MenuBar::initialize()
{
	BC_Resources *resources = get_resources();
// Initialize dimensions
	h = calculate_height(this);
	bg_color = resources->menu_up;

	if(resources->menu_bar_bg) menu_bar_bg = new BC_Pixmap(this,
		resources->menu_bar_bg);

	if(resources->menu_title_bg)
	{
		for(int i = 0; i < 3; i++)
			menu_title_bg[i] = new BC_Pixmap(this,
				resources->menu_title_bg[i]);
	}

// Create the subwindow
	BC_SubWindow::initialize();

	if(resources->menu_bg) 
		set_background(resources->menu_bg);
	draw_face();
	return 0;
}

int BC_MenuBar::calculate_height(BC_WindowBase *window)
{
	if(get_resources()->menu_bar_bg)
		return get_resources()->menu_bar_bg->get_h();
	else
		return window->get_text_height(MEDIUMFONT) + 8;
}

void BC_MenuBar::draw_items()
{
	for(int i = 0; i < menu_titles.total; i++)
		menu_titles.values[i]->draw_items();
	flush();
}

int BC_MenuBar::add_menu(BC_Menu* menu)
{
	int x, w;

// Get dimensions
	if(menu_titles.total == 0)
		x = 2;
	else
		x = menu_titles.values[menu_titles.total - 1]->x + 
			menu_titles.values[menu_titles.total - 1]->w;

	w = get_text_width(MEDIUMFONT, menu->text) + 20;
// get pointer
	menu_titles.append(menu);
// initialize and draw
	menu->initialize(top_level, this, x, 2, w, get_h() - 4); 
	return 0;
}

int BC_MenuBar::focus_out_event()
{
	deactivate();
	return 0;
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
//printf("BC_MenuBar::button_release_event %d\n", result);

	return result;
}

int BC_MenuBar::resize_event(int w, int h)
{
	resize_window(w, get_h());
	draw_face();
	for(int i = 0; i < menu_titles.total; i++)
	{
		menu_titles.values[i]->draw_title();
	}
	return 0;
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

int BC_MenuBar::cursor_leave_event()
{
	for(int i = 0; i < menu_titles.total; i++)
	{
		menu_titles.values[i]->dispatch_cursor_leave();
	}
	return 0;
}

int BC_MenuBar::cursor_enter_event()
{
	if(active) return 1;
	return 0;
}

int BC_MenuBar::translation_event()
{
	if(active)
	{
		for(int i = 0; i < menu_titles.total; i++)
		{
			menu_titles.values[i]->dispatch_translation_event();
		}
	}
	return 0;
}

int BC_MenuBar::activate()
{
	top_level->deactivate();
	top_level->active_menubar = this;
	active = 1;
	return 0;
}

int BC_MenuBar::deactivate()
{
	for(int i = 0; i < menu_titles.total; i++)
	{
		menu_titles.values[i]->deactivate_menu();
	}
	top_level->active_menubar = 0;
	active = 0;
	return 0;
}

int BC_MenuBar::unhighlight()
{
	for(int i = 0; i < menu_titles.total; i++)
	{
		menu_titles.values[i]->unhighlight();
	}
	return 0;
}

int BC_MenuBar::draw_face()
{
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

		set_color(top_level->get_resources()->menu_light);
		draw_line(0, 0, 0, uy);
		draw_line(0, 0, ux, 0);

		set_color(top_level->get_resources()->menu_shadow);
		draw_line(ux, ly, ux, uy);
		draw_line(lx, uy, ux, uy);
		set_color(BLACK);
		draw_line(w, 0, w, h);
		draw_line(0, h, w, h);
	}

	flash();
	flush();
	return 0;
}






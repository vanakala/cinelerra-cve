// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcmenubar.h"
#include "bcmenuitem.h"
#include "bcmenupopup.h"
#include "bcpixmap.h"
#include "bcpopupmenu.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "colors.h"
#include "fonts.h"
#include <string.h>
#include "vframe.h"

#define BUTTON_UP 0
#define BUTTON_HI 1
#define BUTTON_DN 2
#define TOTAL_IMAGES 3

#define TRIANGLE_W 10
#define TRIANGLE_H 10

BC_PopupMenu::BC_PopupMenu(int x, 
		int y, 
		int w, 
		const char *text, 
		int options,
		VFrame **data,
		int margin)
 : BC_SubWindow(x, y, w, 0, -1)
{
	highlighted = popup_down = 0;
	icon = 0;
	if(margin >= 0)
		this->margin = margin;
	else
		this->margin = resources.popupmenu_margin;

	use_coords = options & POPUPMENU_USE_COORDS;
	use_title = 0;
	this->text[0] = 0;
	if(text && *text)
	{
		strcpy(this->text, text);
		use_title = 1;
	}
	for(int i = 0; i < TOTAL_IMAGES; i++)
	{
		images[i] = 0;
	}
	this->data = data;
	this->w_argument = w;
	status = BUTTON_UP;
	menu_popup = new BC_MenuPopup;
}

BC_PopupMenu::BC_PopupMenu(int x, 
		int y, 
		const char *text, 
		int options,
		VFrame **data)
 : BC_SubWindow(x, y, 0, 0, -1)
{
	highlighted = popup_down = 0;
	icon = 0;
	use_title = 0;
	use_coords = options & POPUPMENU_USE_COORDS;
	this->text[0] = 0;
	if(text && *text)
	{
		strcpy(this->text, text);
		use_title = 1;
	}
	for(int i = 0; i < TOTAL_IMAGES; i++)
	{
		images[i] = 0;
	}
	this->data = data;
	this->w_argument = 0;
	status = BUTTON_UP;
	menu_popup = new BC_MenuPopup;
}

BC_PopupMenu::~BC_PopupMenu()
{
	if(menu_popup) delete menu_popup;
	for(int i = 0; i < TOTAL_IMAGES; i++)
	{
		if(images[i]) delete images[i];
	}
}

char* BC_PopupMenu::get_text()
{
	return text;
}

void BC_PopupMenu::set_text(const char *text)
{
	this->text[0] = 0;
	if(text && *text)
		strcpy(this->text, text);
	if(top_level)
		draw_title();
	use_title = 1;
}

void BC_PopupMenu::set_icon(BC_Pixmap *icon)
{
	this->icon = icon;
	if(top_level)
		draw_title();
	use_title = 1;
}

void BC_PopupMenu::initialize()
{
	if(use_title)
	{
		if(data)
			set_images(data);
		else
		if(resources.popupmenu_images)
			set_images(resources.popupmenu_images);
		else
			set_images(resources.generic_button_images);
	}
	else
// Move outside window if no title
	{
		if(!use_coords)
		{
			x = -10;
			y = -10;
		}
		w = 10;
		h = 1;
	}
	BC_SubWindow::initialize();

	menu_popup->initialize(top_level, 
		0, 
		0, 
		0, 
		this);

	if(use_title) draw_title();
}

void BC_PopupMenu::set_images(VFrame **data)
{
	for(int i = 0; i < 3; i++)
	{
		if(images[i]) delete images[i];
		images[i] = new BC_Pixmap(parent_window, data[i], PIXMAP_ALPHA);
	}

	if(w_argument > 0)
		w = w_argument + 
			margin +
			resources.popupmenu_triangle_margin;
	else
		w = get_text_width(MEDIUMFONT, text) + 
			margin +
			resources.popupmenu_triangle_margin;

	h = images[BUTTON_UP]->get_h();
}

int BC_PopupMenu::calculate_h(VFrame **data)
{
	if(data)
		;
	else
	if(resources.popupmenu_images)
		data = resources.popupmenu_images;
	else
		data = resources.generic_button_images;

	return data[BUTTON_UP]->get_h();
}

void BC_PopupMenu::add_item(BC_MenuItem *item)
{
	menu_popup->add_item(item);
}

void BC_PopupMenu::remove_item(BC_MenuItem *item)
{
	menu_popup->remove_item(item);
}

int BC_PopupMenu::total_items()
{
	return menu_popup->total_menuitems();
}

BC_MenuItem* BC_PopupMenu::get_item(int i)
{
	return menu_popup->menu_items.values[i];
}

void BC_PopupMenu::draw_title()
{
	if(!use_title) return;

	top_level->lock_window("BC_PopupMenu::draw_title");
// Background
	draw_top_background(parent_window, 0, 0, w, h);
	set_color(resources.popup_title_text);
	draw_3segmenth(0, 0, w, images[status]);

// Overlay text
	int offset = 0;
	if(status == BUTTON_DN)
		offset = 1;
	if(!icon)
	{
		set_font(MEDIUMFONT);
		BC_WindowBase::draw_center_text(
			(get_w() - margin * 2 - resources.popupmenu_triangle_margin) / 2 + margin + offset,
			(int)((float)get_h() / 2 + get_text_ascent(MEDIUMFONT) / 2 - 2) + offset, 
			text);
	}

	if(icon)
	{
		draw_pixmap(icon,
			(get_w() - margin * 2 - resources.popupmenu_triangle_margin) / 2 + margin + offset - icon->get_w() / 2,
			get_h() / 2 - icon->get_h() / 2 + offset);
	}

	draw_triangle_down_flat(get_w() - margin - resources.popupmenu_triangle_margin,
		get_h() / 2 - TRIANGLE_H / 2, 
		TRIANGLE_W, TRIANGLE_H);

	flash();
	top_level->unlock_window();
}

void BC_PopupMenu::deactivate()
{
	if(popup_down)
	{
		top_level->active_popup_menu = 0;
		popup_down = 0;
		menu_popup->deactivate_menu();

		if(use_title) draw_title();    // draw the title
	}
}

void BC_PopupMenu::activate_menu(int init_releases)
{
	if(!popup_down)
	{
		int x = this->x;
		int y = this->y;

		top_level->deactivate();
		top_level->active_popup_menu = this;
		top_level->lock_window("BC_PopupMenu::activate_menu");
		if(!use_coords)
		{
			int cx, cy;

			BC_Resources::get_abs_cursor(&cx, &cy);
			x = cx - get_w();
			y = cy - get_h();
			button_press_x = top_level->cursor_x;
			button_press_y = top_level->cursor_y;
		}
		button_releases = init_releases;
		if(use_title)
		{
			Window tempwin;
			int new_x, new_y, top_w, top_h;
			XTranslateCoordinates(top_level->display, 
				win, 
				top_level->rootwin, 
				0, 
				0, 
				&new_x, 
				&new_y, 
				&tempwin);
			menu_popup->activate_menu(new_x, 
				new_y, 
				w, 
				h, 
				0, 
				1);
		}
		else
		if(!use_coords)
			menu_popup->activate_menu(x, y, w, h, 0, 1);
		else
			menu_popup->activate_menu(x, y, w, h, 1, 1);
		popup_down = 1;
		top_level->unlock_window();
		if(use_title) draw_title();
	}
}

void BC_PopupMenu::deactivate_menu()
{
	deactivate();
}

void BC_PopupMenu::reposition_window(int x, int y)
{
	BC_WindowBase::reposition_window(x, y);
	draw_title();
}

void BC_PopupMenu::focus_out_event()
{
	deactivate();
}

void BC_PopupMenu::repeat_event(int duration)
{
	if(duration == resources.tooltip_delay &&
		tooltip_wtext &&
		status == BUTTON_HI &&
		!tooltip_done)
	{
		show_tooltip();
		tooltip_done = 1;
	}
}

int BC_PopupMenu::button_press_event()
{
	int result = 0;

	if(get_buttonpress() == 1 &&
		is_event_win() && 
		use_title)
	{
		top_level->hide_tooltip();
		if(status == BUTTON_HI || status == BUTTON_UP) status = BUTTON_DN;
		activate_menu();
		draw_title();
		return 1;
	}

	// Scrolling section
	if (is_event_win() 
		&& (get_buttonpress() == 4 || get_buttonpress() == 5) 
		&& menu_popup->total_menuitems() > 1)
	{
		int theval = -1;
		for (int i = 0; i < menu_popup->total_menuitems(); i++) {
			if (!strcmp(menu_popup->menu_items.values[i]->get_text(),get_text())) {
				theval=i; 
				break;
			}
		}

		if (theval == -1)                  theval=0;
		else if (get_buttonpress() == 4)   theval--;
		else if (get_buttonpress() == 5)   theval++;

		if (theval < 0)
			theval=0;
		if (theval >= menu_popup->total_menuitems()) 
			theval = menu_popup->total_menuitems() - 1;

		BC_MenuItem *tmp = menu_popup->menu_items.values[theval];
		set_text(tmp->get_text());
		if (!tmp->handle_event())
			this->handle_event();
	}

	if(popup_down)
	{
// Menu is down so dispatch to popup.
		menu_popup->dispatch_button_press();
		return 1;
	}

	return 0;
}

int BC_PopupMenu::button_release_event()
{
// try the title
	int result = 0;

	button_releases++;

	if(is_event_win() && use_title)
	{
		hide_tooltip();
		if(status == BUTTON_DN)
		{
			status = BUTTON_HI;
			draw_title();
		}
	}

	if(popup_down)
	{
// Menu is down so dispatch to popup.
		result = menu_popup->dispatch_button_release();
	}

	if(popup_down && button_releases >= 2)
	{
		deactivate();
	}

	if(!result && use_title && cursor_inside() && is_event_win())
	{
		hide_tooltip();
		result = 1;
	}
	else
	if(!result && !use_title && popup_down && button_releases < 2)
	{
		result = 1;
	}

	if(!result && popup_down)
	{
// Button was released outside any menu.
		deactivate();
		result = 1;
	}
	return result;
}

void BC_PopupMenu::translation_event()
{
	if(popup_down) menu_popup->dispatch_translation_event();
}

void BC_PopupMenu::cursor_leave_event()
{

	if(status == BUTTON_HI && use_title)
	{
		status = BUTTON_UP;
		draw_title();
		hide_tooltip();
	}

// dispatch to popup
	if(popup_down)
	{
		menu_popup->dispatch_cursor_leave();
	}
}

int BC_PopupMenu::cursor_enter_event()
{
	if(is_event_win() && use_title)
	{
		tooltip_done = 0;
		if(top_level->button_down)
		{
			status = BUTTON_DN;
		}
		else
		if(status == BUTTON_UP) 
			status = BUTTON_HI;
		draw_title();
	}
	return 0;
}

int BC_PopupMenu::cursor_motion_event()
{
	int result = 0;

// This menu is down.
	if(popup_down)
	{
		result = menu_popup->dispatch_motion_event();
	}

	if(!result && use_title && top_level->event_win == win)
	{
		if(highlighted)
		{
			if(cursor_inside())
			{
				highlighted = 0;
				draw_title();
			}
		}
		else
		{
			if(cursor_inside())
			{
				highlighted = 1;
				draw_title();
				result = 1;
			}
		}
	}

	return result;
}

int BC_PopupMenu::drag_start_event()
{
	if(popup_down) return 1;
	return 0;
}

// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcmenubar.h"
#include "bcmenuitem.h"
#include "bcmenupopup.h"
#include "bcpixmap.h"
#include "bcpopup.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcwindowbase.h"
#include "clip.h"

#include <string.h>



// ==================================== Menu Popup =============================

// Types of menu popups
#define MENUPOPUP_MENUBAR 0
#define MENUPOPUP_SUBMENU 1
#define MENUPOPUP_POPUP   2

BC_MenuPopup::BC_MenuPopup()
{
	window_bg = 0;
	item_bg[0] = 0;
	item_bg[1] = 0;
	item_bg[2] = 0;
	top_level = 0;
}

BC_MenuPopup::~BC_MenuPopup()
{
	while(menu_items.total)
	{
// Each menuitem recursively removes itself from the arraylist
		delete menu_items.values[0];
	}
	delete window_bg;
	delete item_bg[0];
	delete item_bg[1];
	delete item_bg[2];
}

void BC_MenuPopup::initialize(BC_WindowBase *top_level, 
		BC_MenuBar *menu_bar, 
		BC_Menu *menu, 
		BC_MenuItem *menu_item, 
		BC_PopupMenu *popup_menu)
{
	popup = 0;
	active = 0;
	this->menu = menu;
	this->menu_bar = menu_bar;
	this->menu_item = menu_item;
	this->popup_menu = popup_menu;
	this->top_level = top_level;

	if(menu_item) this->type = MENUPOPUP_SUBMENU;
	else
	if(menu) this->type = MENUPOPUP_MENUBAR;
	else
	if(popup_menu) this->type = MENUPOPUP_POPUP;

	BC_Resources *resources = top_level->get_resources();
	if(resources->menu_popup_bg)
	{
		window_bg = new BC_Pixmap(top_level, resources->menu_popup_bg);
	}
	if(resources->menu_item_bg)
	{
		item_bg[0] = new BC_Pixmap(top_level, resources->menu_item_bg[0], PIXMAP_ALPHA);
		item_bg[1] = new BC_Pixmap(top_level, resources->menu_item_bg[1], PIXMAP_ALPHA);
		item_bg[2] = new BC_Pixmap(top_level, resources->menu_item_bg[2], PIXMAP_ALPHA);
	}
	for(int i = 0; i < menu_items.total; i++)
	{
		BC_MenuItem *item = menu_items.values[i];
		item->initialize(top_level, menu_bar, this);
	}
}

void BC_MenuPopup::add_item(BC_MenuItem *item)
{
	menu_items.append(item);
	if(top_level)
		item->initialize(top_level, menu_bar, this);
// items uninitialized here will be initialized later in initialize
}

void BC_MenuPopup::remove_item(BC_MenuItem *item)
{
	if(!item)
	{
		item = menu_items.values[menu_items.total - 1];
		delete item;
	}
	if(item) menu_items.remove(item);
}

int BC_MenuPopup::total_menuitems()
{
	return menu_items.total;
}

int BC_MenuPopup::dispatch_button_press()
{
	int result = 0;
	if(popup)
	{
		for(int i = 0; i < menu_items.total && !result; i++)
		{
			result = menu_items.values[i]->dispatch_button_press();
		}
		if(result) draw_items();
	}
	return 0;
}

int BC_MenuPopup::dispatch_button_release()
{
	int result = 0, redraw = 0;
	if(popup)
	{
		for(int i = 0; i < menu_items.total && !result; i++)
		{
			result = menu_items.values[i]->dispatch_button_release(redraw);
		}
		if(redraw) draw_items();
	}
	return result;
}

int BC_MenuPopup::dispatch_key_press()
{
	int result = 0;
	for(int i = 0; i < menu_items.total && !result; i++)
	{
		result = menu_items.values[i]->dispatch_key_press();
	}
	return result;
}

int BC_MenuPopup::dispatch_motion_event()
{
	int i, result = 0, redraw = 0;
	Window tempwin;

	if(popup)
	{
// Try submenus and items
		for(i = 0; i < menu_items.total; i++)
		{
			result |= menu_items.values[i]->dispatch_motion_event(redraw);
		}

		if(redraw) draw_items();
	}

	return result;
}

void BC_MenuPopup::dispatch_translation_event()
{
	if(popup)
	{
		int new_x = x + 
			(top_level->last_translate_x - 
			top_level->prev_x - 
			top_level->get_resources()->get_left_border());
		int new_y = y + 
			(top_level->last_translate_y - 
			top_level->prev_y -
			top_level->get_resources()->get_top_border());

		popup->reposition_window(new_x, new_y, popup->get_w(), popup->get_h());
		top_level->flush();
		this->x = new_x;
		this->y = new_y;

		for(int i = 0; i < menu_items.total; i++)
		{
			menu_items.values[i]->dispatch_translation_event();
		}
	}
}

void BC_MenuPopup::dispatch_cursor_leave()
{
	if(popup)
	{
		for(int i = 0; i < menu_items.total; i++)
		{
			menu_items.values[i]->dispatch_cursor_leave();
		}
		draw_items();
	}
}

void BC_MenuPopup::activate_menu(int x, 
	int y, 
	int w, 
	int h, 
	int top_window_coords, 
	int vertical_justify)
{
	Window tempwin;
	int new_x, new_y, top_w, top_h;
	top_w = top_level->get_root_w(1, 0);
	top_h = top_level->get_root_h(0);

	get_dimensions();

// Coords are relative to the main window
	if(top_window_coords)
	{
		top_level->lock_window("BC_MenuPopup::activate_menu");
		XTranslateCoordinates(top_level->display, 
			top_level->win, 
			top_level->rootwin, 
			x, 
			y, 
			&new_x, 
			&new_y, 
			&tempwin);
		top_level->unlock_window();
	} 
	else
// Coords are absolute
	{
		new_x = x; 
		new_y = y; 
	}

// All coords are now relative to root window.
	if(vertical_justify)
	{
		this->x = new_x;
		this->y = new_y + h;
		if(this->x + this->w > top_w) this->x -= this->x + this->w - top_w; // Right justify
		if(this->y + this->h > top_h) this->y -= this->h + h; // Bottom justify
// Avoid top of menu going out of screen
		if(this->y < 0)
			this->y = 20;
	}
	else
	{
		this->x = new_x + w;
		this->y = new_y;
		if(this->x + this->w > top_w) this->x = new_x - this->w;
		if(this->y + this->h > top_h) this->y = new_y + h - this->h;
	}

	active = 1;
	if(menu_bar)
	{
		popup = new BC_Popup(menu_bar, 
					this->x, 
					this->y, 
					this->w, 
					this->h, 
					top_level->get_resources()->menu_up,
					1,
					menu_bar->bg_pixmap);
	}
	else
	{
		popup = new BC_Popup(top_level, 
					this->x, 
					this->y, 
					this->w, 
					this->h, 
					top_level->get_resources()->menu_up,
					1,
					0);
	}
	draw_items();
	popup->show_window();
}

void BC_MenuPopup::deactivate_submenus(BC_MenuPopup *exclude)
{
	for(int i = 0; i < menu_items.total; i++)
	{
		menu_items.values[i]->deactivate_submenus(exclude);
	}
}

void BC_MenuPopup::deactivate_menu()
{
	deactivate_submenus(0);

	if(popup) delete popup;
	popup = 0;
	active = 0;
}

void BC_MenuPopup::draw_items()
{
	top_level->lock_window("BC_MenuPopup::draw_items");
	if(menu_bar)
		popup->draw_top_tiles(menu_bar, 0, 0, w, h);
	else
		popup->draw_top_tiles(popup, 0, 0, w, h);

	if(window_bg)
	{
		popup->draw_9segment(0,
			0,
			w,
			h,
			window_bg);
	}
	else
	{
		popup->draw_3d_border(0, 0, w, h, 
			top_level->get_resources()->menu_light,
			top_level->get_resources()->menu_up,
			top_level->get_resources()->menu_shadow,
			BLACK);
	}

	for(int i = 0; i < menu_items.total; i++)
	{
		menu_items.values[i]->draw();
	}
	popup->flash();
	top_level->unlock_window();
}

void BC_MenuPopup::get_dimensions()
{
	int widest_text = 10, widest_key = 10;
	int text_w, key_w;
	int i = 0;

// pad for border
	h = 2;
// Set up parameters in each item and get total h. 
	for(i = 0; i < menu_items.total; i++)
	{
		text_w = 10 + top_level->get_text_width(MEDIUMFONT, menu_items.values[i]->text);
		if(menu_items.values[i]->checked) text_w += 20;

		key_w = 10 + top_level->get_text_width(MEDIUMFONT, menu_items.values[i]->hotkey_text);
		if(text_w > widest_text) widest_text = text_w;
		if(key_w > widest_key) widest_key = key_w;

		if(!strcmp(menu_items.values[i]->text, "-")) 
			menu_items.values[i]->h = 5;
		else
			menu_items.values[i]->h = top_level->get_text_height(MEDIUMFONT) + 4;

		menu_items.values[i]->y = h;
		menu_items.values[i]->highlighted = 0;
		menu_items.values[i]->down = 0;
		h += menu_items.values[i]->h;
	}
	w = widest_text + widest_key + 10;

	w = MAX(w, top_level->get_resources()->min_menu_w);
// pad for division
	key_x = widest_text + 5;
// pad for border
	h += 2;
}

int BC_MenuPopup::get_key_x()
{
	return key_x;
}

BC_Popup* BC_MenuPopup::get_popup()
{
	return popup;
}

int BC_MenuPopup::get_w()
{
	return w;
}


// ================================= Sub Menu ==================================

BC_SubMenu::BC_SubMenu() : BC_MenuPopup()
{
}

BC_SubMenu::~BC_SubMenu()
{
}

void BC_SubMenu::add_submenuitem(BC_MenuItem *item)
{
	add_item(item);
}

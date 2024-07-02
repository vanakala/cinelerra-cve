// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BCMENU_H
#define BCMENU_H

#include "bcmenubar.inc"
#include "bcmenuitem.inc"
#include "bcmenupopup.inc"
#include "bcwindowbase.inc"

// Subscripts for menu images
#define MENU_BG 0
#define MENU_ITEM_UP 1
#define MENU_ITEM_HI 1
#define MENU_ITEM_DN 1

class BC_Menu
{
public:
	BC_Menu(char *text);
	virtual ~BC_Menu();

	friend class BC_MenuBar;

// Called by user to add items
	void add_item(BC_MenuItem* menuitem);
// Remove the item ptr and the object
	void remove_item(BC_MenuItem* item = 0);
	int total_menuitems();
	void set_text(const char *text);

// Called by BC_Menubar
	void initialize(BC_WindowBase *top_level, BC_MenuBar *menu_bar, int x, int y, int w, int h);
	int dispatch_button_press();
	int dispatch_button_release();
	int dispatch_keypress();
	int dispatch_motion_event();
	void dispatch_cursor_leave();
	void dispatch_translation_event();
	void deactivate_menu();
	void activate_menu();
	void unhighlight();
	void draw_items();

private:
	void draw_title();
// If this menu is pulled down
	int active;
	char text[1024];
	BC_WindowBase *top_level;
// Owner menubar if there is one
	BC_MenuBar *menu_bar;
// Client popup
	BC_MenuPopup *menu_popup;
	int highlighted;
// Dimensions relative to menubar.
	int x, y, w, h; 
};

#endif

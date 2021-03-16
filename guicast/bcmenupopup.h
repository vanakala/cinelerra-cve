// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BCMENUPOPUP_H
#define BCMENUPOPUP_H

#include "arraylist.h"
#include "bcmenu.inc"
#include "bcmenubar.inc"
#include "bcmenuitem.inc"
#include "bcpopup.inc"
#include "bcpopupmenu.inc"
#include "bcwindowbase.inc"

// A window that contains a menu.
class BC_MenuPopup
{
public:
	BC_MenuPopup();
	virtual ~BC_MenuPopup();

	friend class BC_MenuItem;
	friend class BC_PopupMenu;

	void initialize(BC_WindowBase *top_level, 
		BC_MenuBar *menu_bar, 
		BC_Menu *menu, 
		BC_MenuItem *menu_item, 
		BC_PopupMenu *popup_menu);
	void add_item(BC_MenuItem *item);
	void remove_item(BC_MenuItem* item = 0);
	int total_menuitems();

// Deactivates all submenus in a downward progression except for the exclude
	void deactivate_submenus(BC_MenuPopup *exclude = 0);
	int dispatch_button_press();
	int dispatch_button_release();
	int dispatch_key_press();
	int dispatch_motion_event();
	void dispatch_cursor_leave();
	void dispatch_translation_event();
	void deactivate_menu();
	void activate_menu(int x, int y, int w, int h, int top_window_coords, int vertical_justify);
	int get_key_x();
	int get_w();
	void draw_items();
	BC_Popup* get_popup();

private:
	void get_dimensions();

	ArrayList<BC_MenuItem *> menu_items;  
	BC_WindowBase *top_level;
	BC_MenuItem *menu_item;
	BC_MenuBar *menu_bar;
	BC_PopupMenu *popup_menu;
	BC_Menu *menu;
// Dimensions relative to root window
	int x, y, w, h; 
// Horizontal position of hotkey text
	int key_x;
// Popup window that only exists when menu is down.
	BC_Popup *popup; 
	int active;
	int type;
// Images for backgrounds
	BC_Pixmap *window_bg;
	BC_Pixmap *item_bg[3];
};

class BC_SubMenu : public BC_MenuPopup
{
public:
	BC_SubMenu();
	virtual ~BC_SubMenu();

	void add_submenuitem(BC_MenuItem *item);
};

#endif

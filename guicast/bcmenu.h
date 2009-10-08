
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
	int add_item(BC_MenuItem* menuitem);
// Remove the item ptr and the object
	int remove_item(BC_MenuItem* item = 0);
	int total_menuitems();
	int set_text(char *text);

// Called by BC_Menubar
	int initialize(BC_WindowBase *top_level, BC_MenuBar *menu_bar, int x, int y, int w, int h);
	int dispatch_button_press();
	int dispatch_button_release();
	int dispatch_keypress();
	int dispatch_motion_event();
	int dispatch_cursor_leave();
	int dispatch_translation_event();
	int deactivate_menu();
	int activate_menu();
	int unhighlight();
	void draw_items();

private:
	int draw_title();
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


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

#ifndef BCMENUITEM_H
#define BCMENUITEM_H


#include "bcmenubar.inc"
#include "bcmenupopup.inc"
#include "bcpixmap.inc"
#include "bcpopupmenu.inc"
#include "bcwindowbase.inc"



class BC_MenuItem
{
public:
	BC_MenuItem(char *text, char *hotkey_text = "", int hotkey = 0);
	virtual ~BC_MenuItem();

	friend class BC_MenuPopup;

	void reset();
	int add_submenu(BC_SubMenu *submenu);
	int set_checked(int value);
	int get_checked();
	void set_text(char *text);
	char* get_text();
	void set_icon(BC_Pixmap *icon);
	BC_Pixmap* get_icon();
	void set_hotkey_text(char *text);
	int set_shift(int value = 1);
	int set_alt(int value = 1);

	int deactivate_submenus(BC_MenuPopup *exclude);
	int activate_submenu();
	virtual int handle_event() { return 0; };
	int dispatch_button_press();
	int dispatch_button_release(int &redraw);
	int dispatch_motion_event(int &redraw);
	int dispatch_translation_event();
	int dispatch_cursor_leave();
	int dispatch_key_press();
	int add_item(BC_MenuItem *item);
	int initialize(BC_WindowBase *top_level, BC_MenuBar *menu_bar, BC_MenuPopup *menu_popup);
	int draw();
	BC_WindowBase* get_top_level();
	BC_PopupMenu* get_popup_menu();

private:
	BC_WindowBase *top_level;
	BC_MenuBar *menu_bar;
	BC_MenuPopup *menu_popup;
// Submenu if this item owns one.
	BC_SubMenu *submenu;          
// whether the cursor is over or not
	int highlighted;
// whether the cursor is over and the button is down
	int down;
// check box
	int checked;
// title
	char *text;
// text of hotkey
	char *hotkey_text;
// Hotkey requires shift
	int shift_hotkey;
// Hotkey requires alt
	int alt_hotkey;
// Character code of hotkey
	int hotkey;
// icon or 0 if there is no icon
	BC_Pixmap *icon;
// y position of this item set during menu activation
	int y;
// height of item is set during menu activation
	int h;
};



#endif

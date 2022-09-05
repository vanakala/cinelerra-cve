// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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
	BC_MenuItem(const char *text, const char *hotkey_text = "", int hotkey = 0);
	virtual ~BC_MenuItem();

	friend class BC_MenuPopup;

	void add_submenu(BC_SubMenu *submenu);
	void set_checked(int value);
	int get_checked();
	void set_text(const char *text);
	char* get_text();
	void set_icon(BC_Pixmap *icon);
	BC_Pixmap* get_icon();
	void set_hotkey_text(const char *text);
	void set_shift(int value = 1);
	void set_alt(int value = 1);
	void disable();
	void enable();
	int get_disabled() { return disabled; };

	void deactivate_submenus(BC_MenuPopup *exclude);
	void activate_submenu();
	virtual int handle_event() { return 0; };
	int dispatch_button_press();
	int dispatch_button_release(int &redraw);
	int dispatch_motion_event(int &redraw);
	void dispatch_translation_event();
	void dispatch_cursor_leave();
	int dispatch_key_press();
	void initialize(BC_WindowBase *top_level, BC_MenuBar *menu_bar, BC_MenuPopup *menu_popup);
	void draw();
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
// Item is disabled
	int disabled;
};

#endif

// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BCPOPUPMENU_H
#define BCPOPUPMENU_H

#include "bcmenubar.inc"
#include "bcmenuitem.inc"
#include "bcmenupopup.inc"
#include "bcsubwindow.h"

#define POPUPMENU_TOTAL_IMAGES 3

// A menu that pops up in the middle of a window or under a button.

class BC_PopupMenu : public BC_SubWindow
{
public:
	BC_PopupMenu(int x, int y, int w,
		const char *text, int options = 0,
// Data for alternative title images
		VFrame **data = 0,
// Alternative text margin
		int margin = -1);
	BC_PopupMenu(int x, int y,
		const char *text, int options = 0,
// Data for alternative title images
		VFrame **data = 0);
	virtual ~BC_PopupMenu();

	static int calculate_h(VFrame **data = 0);
	virtual int handle_event() { return 0; };
	char* get_text();
	void initialize();
	void add_item(BC_MenuItem *item);
	void remove_item(BC_MenuItem *item);
	int total_items();
	BC_MenuItem* get_item(int i);
// Set title of menu
	void set_text(const char *text);
// Set icon of menu.  Disables text.
	void set_icon(BC_Pixmap *pixmap);
// Draw title of menu
	void draw_title();
	void reposition_window(int x, int y);
	void deactivate();
	void activate_menu(int init_releases = 0);
	void deactivate_menu();
	void focus_out_event();
	void repeat_event(int duration);
	int button_press_event();
	int button_release_event();
	void cursor_leave_event();
	int cursor_enter_event();
	int cursor_motion_event();
	void translation_event();
	int drag_start_event();
	void set_images(VFrame **data);

private:
	void init_images();
	char text[BCTEXTLEN];
	int margin;
	VFrame **data;
	BC_Pixmap *images[POPUPMENU_TOTAL_IMAGES];
	BC_Pixmap *icon;
	int highlighted;
	int popup_down;
	int use_title;
	int use_coords;
	int button_releases;
	BC_MenuPopup *menu_popup;
// Remember cursor position when no title
	int button_press_x, button_press_y;
	int w_argument;
	int status;
};



#endif


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

#ifndef BCPOPUPMENU_H
#define BCPOPUPMENU_H

#include "bcmenubar.inc"
#include "bcmenuitem.inc"
#include "bcmenupopup.inc"
#include "bcsubwindow.h"


// A menu that pops up in the middle of a window or under a button.



class BC_PopupMenu : public BC_SubWindow
{
public:
	BC_PopupMenu(int x, 
		int y, 
		int w, 
		char *text, 
		int use_title = 1,
// Data for alternative title images
		VFrame **data = 0,
// Alternative text margin
		int margin = -1);
	BC_PopupMenu(int x, 
		int y, 
		char *text, 
		int use_title = 1,
// Data for alternative title images
		VFrame **data = 0);
	virtual ~BC_PopupMenu();

	static int calculate_h(VFrame **data = 0);
	virtual int handle_event() { return 0; };
	char* get_text();
	int initialize();
	int add_item(BC_MenuItem *item);
	int remove_item(BC_MenuItem *item);
	int total_items();
	BC_MenuItem* get_item(int i);
// Set title of menu
	void set_text(char *text);
// Set icon of menu.  Disables text.
	void set_icon(BC_Pixmap *pixmap);
// Draw title of menu
	int draw_title();
	int reposition_window(int x, int y);
	int deactivate();
	int activate_menu();
	int deactivate_menu();
	int focus_out_event();
	int repeat_event(int64_t duration);
	int button_press_event();
	int button_release_event();
	int cursor_leave_event();
	int cursor_enter_event();
	int cursor_motion_event();
	int translation_event();
	int drag_start_event();
	int drag_stop_event();
	int drag_motion_event();
	int set_images(VFrame **data);

private:
	char text[BCTEXTLEN];
	int margin;
	VFrame **data;
	BC_Pixmap *images[9];
	BC_Pixmap *icon;
	int highlighted;
	int popup_down;
	int use_title;
	int button_releases;
	BC_MenuPopup *menu_popup;
// Remember cursor position when no title
	int button_press_x, button_press_y;
	int w_argument;
	int status;
};



#endif

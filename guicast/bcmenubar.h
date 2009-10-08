
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

#ifndef BCMENUBAR_H
#define BCMENUBAR_H

#include "bcmenu.inc"
#include "bcmenubar.inc"
#include "bcpixmap.inc"
#include "bcsubwindow.h"

class BC_MenuBar : public BC_SubWindow
{
public:
	BC_MenuBar(int x, int y, int w);
	virtual ~BC_MenuBar();

	friend class BC_Menu;

	int add_menu(BC_Menu* menu);
	static int calculate_height(BC_WindowBase *window);

	int initialize();
	int focus_out_event();
	int keypress_event();
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	int cursor_enter_event();
	int cursor_leave_event();
	int resize_event(int w, int h);
	int translation_event();
	int deactivate();
	int unhighlight();
// Redraws items in active menu
	void draw_items();

private:
	int draw_face();
	int activate();
// Array of menu titles
	ArrayList<BC_Menu*> menu_titles;  
// number of button releases since activation
	int button_releases;        
// When a menu is pulled down
	int active;
	BC_Pixmap *menu_bar_bg;
	BC_Pixmap *menu_title_bg[3];
};





#endif

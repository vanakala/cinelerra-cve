// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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

	void add_menu(BC_Menu* menu);
	static int calculate_height(BC_WindowBase *window);

	void initialize();
	void focus_out_event();
	int keypress_event();
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	int cursor_enter_event();
	void cursor_leave_event();
	void resize_event(int w, int h);
	void translation_event();
	void deactivate();
	void unhighlight();
// Redraws items in active menu
	void draw_items();

private:
	void draw_face();
	void activate();
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

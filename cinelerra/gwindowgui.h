
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

#ifndef GWINDOWGUI_H
#define GWINDOWGUI_H

#include "automation.inc"
#include "guicast.h"
#include "mwindow.inc"

class GWindowToggle;

enum {
	NONAUTOTOGGLES_ASSETS,
	NONAUTOTOGGLES_TITLES,
	NONAUTOTOGGLES_TRANSITIONS,
	NONAUTOTOGGLES_PLUGIN_AUTOS,
	NONAUTOTOGGLES_COUNT
};

struct toggleinfo
{
	int isauto;
	int ref;
};

class GWindowGUI : public BC_Window
{
public:
	GWindowGUI(MWindow *mwindow, int w, int h);
	static void calculate_extents(BC_WindowBase *gui, int *w, int *h);
	void create_objects();
	int translation_event();
	int close_event();
	int keypress_event();
	void update_toggles(int use_lock);
	void update_mwindow();

	MWindow *mwindow;
	GWindowToggle *toggles[NONAUTOTOGGLES_COUNT + AUTOMATION_TOTAL];
};

class GWindowToggle : public BC_CheckBox
{
public:
	GWindowToggle(MWindow *mwindow, 
		GWindowGUI *gui, 
		int x, 
		int y, 
		toggleinfo toggleinf);
	int handle_event();
	void update();

	static int* get_main_value(MWindow *mwindow, toggleinfo toggleinf);

	MWindow *mwindow;
	GWindowGUI *gui;
	toggleinfo toggleinf;
};

#endif

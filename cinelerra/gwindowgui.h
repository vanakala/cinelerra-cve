// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef GWINDOWGUI_H
#define GWINDOWGUI_H

#include "automation.inc"
#include "bcwindow.h"
#include "bctoggle.h"

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
	GWindowGUI(int w, int h);

	static void calculate_extents(BC_WindowBase *gui, int *w, int *h);
	void translation_event();
	void close_event();
	int keypress_event();
	void update_toggles();

	GWindowToggle *toggles[NONAUTOTOGGLES_COUNT + AUTOMATION_TOTAL];
};

class GWindowToggle : public BC_CheckBox
{
public:
	GWindowToggle(GWindowGUI *gui,
		int x, 
		int y, 
		toggleinfo toggleinf);

	int handle_event();
	void update();

	static int* get_main_value(toggleinfo toggleinf);

	GWindowGUI *gui;
	toggleinfo toggleinf;
};

#endif

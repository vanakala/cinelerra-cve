// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef RGB601WINDOW_H
#define RGB601WINDOW_H

class RGB601Window;

#include "rgb601.h"
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER

class RGB601Direction : public BC_CheckBox
{
public:
	RGB601Direction(RGB601Window *window, int x, int y, int *output,
		int true_value, const char *text);

	int handle_event();
	RGB601Window *window;
	int *output;
	int true_value;
};

class RGB601Window : public PluginWindow
{
public:
	RGB601Window(RGB601Main *plugin, int x, int y);

	void update();

	RGB601Main *client;
	BC_CheckBox *forward, *reverse;
	PLUGIN_GUI_CLASS_MEMBERS
};

#endif

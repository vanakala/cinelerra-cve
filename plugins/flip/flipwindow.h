// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FLIPWINDOW_H
#define FLIPWINDOW_H

#include "filexml.inc"
#include "flip.h"
#include "pluginvclient.h"
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER

class FlipToggle;

class FlipWindow : public PluginWindow
{
public:
	FlipWindow(FlipMain *plugin, int x, int y);

	void update();

	FlipToggle *flip_vertical;
	FlipToggle *flip_horizontal;
	PLUGIN_GUI_CLASS_MEMBERS
};

class FlipToggle : public BC_CheckBox
{
public:
	FlipToggle(FlipMain *client, int *output, char *string, int x, int y);

	int handle_event();

	FlipMain *client;
	int *output;
};

#endif

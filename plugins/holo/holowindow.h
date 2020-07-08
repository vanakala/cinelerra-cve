// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef HOLOWINDOW_H
#define HOLOWINDOW_H

#include "holo.h"
#include "bcslider.h"
#include "pluginclient.h"
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER

class HoloSize : public BC_ISlider
{
public:
	HoloSize(HoloMain *plugin, int x, int y,
		int *output, int min, int max);

	int handle_event();
private:
	HoloMain *plugin;
	int *output;
};

class HoloTime : public BC_FSlider
{
public:
	HoloTime(HoloMain *plugin, int x, int y,
		ptstime *output, ptstime min, ptstime max);

	int handle_event();
private:
	HoloMain *plugin;
	ptstime *output;
};

class HoloWindow : public PluginWindow
{
public:
	HoloWindow(HoloMain *plugin, int x, int y);

	void update();

	PLUGIN_GUI_CLASS_MEMBERS
private:
	HoloSize *threshold;
	HoloTime *recycle;
};

#endif

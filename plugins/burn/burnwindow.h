// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BURNWINDOW_H
#define BURNWINDOW_H

#include "bcslider.h"
#include "burn.h"
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER

class BurnSize : public BC_ISlider
{
public:
	BurnSize(BurnMain *plugin, int x, int y,
		int *output, int min, int max);

	int handle_event();
private:
	BurnMain *plugin;
	int *output;
};


class BurnWindow : public PluginWindow
{
public:
	BurnWindow(BurnMain *plugin, int x, int y);

	void update();
	PLUGIN_GUI_CLASS_MEMBERS
private:
	BurnSize *threshold;
	BurnSize *decay;
};

#endif

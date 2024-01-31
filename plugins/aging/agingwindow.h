// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef AGINGWINDOW_H
#define AGINGWINDOW_H

#include "aging.h"
#include "bcslider.h"
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER

class AgingToggle : public BC_CheckBox
{
public:
	AgingToggle(AgingMain *plugin, int x, int y, int *output,
		const char *string);

	int handle_event();

	AgingMain *plugin;
	int *output;
};

class AgingSize : public BC_ISlider
{
public:
	AgingSize(AgingMain *plugin, int x, int y,
		int *output, int min, int max);

	int handle_event();

	AgingMain *plugin;
	int *output;
};

class AgingWindow : public PluginWindow
{
public:
	AgingWindow(AgingMain *plugin, int x, int y);

	void update();
	PLUGIN_GUI_CLASS_MEMBERS
private:
	AgingToggle *colorage;
	AgingToggle *scratch;
	AgingToggle *pits;
	AgingToggle *dust;
	AgingSize *area_scale;
	AgingSize *scratch_lines;
	AgingSize *dust_interval;
};

#endif

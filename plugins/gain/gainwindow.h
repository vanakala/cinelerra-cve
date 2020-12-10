// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef GAINWINDOW_H
#define GAINWINDOW_H

#include "bcslider.h"
#include "gain.h"
#include "pluginclient.h"
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER

class GainLevel;

class GainWindow : public PluginWindow
{
public:
	GainWindow(Gain *gain, int x, int y);

	void update();

	GainLevel *level;
	PLUGIN_GUI_CLASS_MEMBERS
};

class GainLevel : public BC_FSlider
{
public:
	GainLevel(Gain *gain, int x, int y);
	int handle_event();
	Gain *plugin;
};

#endif

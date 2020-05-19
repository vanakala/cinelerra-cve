// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>


#ifndef DESPIKEWINDOW_H
#define DESPIKEWINDOW_H

#include "bcslider.h"
#include "despike.h"
#include "mutex.h"
#include "pluginclient.h"
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER

class DespikeLevel;
class DespikeSlope;

class DespikeWindow : public PluginWindow
{
public:
	DespikeWindow(Despike *despike, int x, int y);

	void update();

	DespikeLevel *level;
	DespikeSlope *slope;
	PLUGIN_GUI_CLASS_MEMBERS
};

class DespikeLevel : public BC_FSlider
{
public:
	DespikeLevel(Despike *despike, int x, int y);
	int handle_event();
	Despike *despike;
};

class DespikeSlope : public BC_FSlider
{
public:
	DespikeSlope(Despike *despike, int x, int y);
	int handle_event();
	Despike *despike;
};

#endif

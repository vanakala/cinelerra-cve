// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef TIMEAVGWINDOW_H
#define TIMEAVGWINDOW_H

class TimeAvgAccum;
class TimeAvgAvg;
class TimeAvgOr;
class TimeAvgParanoid;
class TimeAvgNoSubtract;
class TimeAvgSlider;

#include "bcslider.h"
#include "timeavg.h"
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER

class TimeAvgWindow : public PluginWindow
{
public:
	TimeAvgWindow(TimeAvgMain *plugin, int x, int y);

	void update();

	TimeAvgSlider *total_frames;
	TimeAvgAccum *accum;
	TimeAvgAvg *avg;
	TimeAvgOr *inclusive_or;
	TimeAvgParanoid *paranoid;
	TimeAvgNoSubtract *no_subtract;
	PLUGIN_GUI_CLASS_MEMBERS
};

class TimeAvgSlider : public BC_FSlider
{
public:
	TimeAvgSlider(TimeAvgMain *client, int x, int y);

	int handle_event();

	TimeAvgMain *client;
};

class TimeAvgAccum : public BC_Radial
{
public:
	TimeAvgAccum(TimeAvgMain *client, TimeAvgWindow *gui, int x, int y);
	int handle_event();
	TimeAvgMain *client;
	TimeAvgWindow *gui;
};

class TimeAvgAvg : public BC_Radial
{
public:
	TimeAvgAvg(TimeAvgMain *client, TimeAvgWindow *gui, int x, int y);
	int handle_event();
	TimeAvgMain *client;
	TimeAvgWindow *gui;
};

class TimeAvgOr : public BC_Radial
{
public:
	TimeAvgOr(TimeAvgMain *client, TimeAvgWindow *gui, int x, int y);
	int handle_event();
	TimeAvgMain *client;
	TimeAvgWindow *gui;
};

class TimeAvgParanoid : public BC_CheckBox
{
public:
	TimeAvgParanoid(TimeAvgMain *client, int x, int y);
	int handle_event();
	TimeAvgMain *client;
};

class TimeAvgNoSubtract : public BC_CheckBox
{
public:
	TimeAvgNoSubtract(TimeAvgMain *client, int x, int y);
	int handle_event();
	TimeAvgMain *client;
};

#endif

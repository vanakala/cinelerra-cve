// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BRIGHTNESSWINDOW_H
#define BRIGHTNESSWINDOW_H

class BrightnessThread;
class BrightnessWindow;
class BrightnessSlider;
class BrightnessLuma;

#include "bcslider.h"
#include "bctoggle.h"
#include "brightness.h"
#include "pluginvclient.h"
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER

class BrightnessWindow : public PluginWindow
{
public:
	BrightnessWindow(BrightnessMain *plugin, int x, int y);

	void update();

	BrightnessSlider *brightness;
	BrightnessSlider *contrast;
	BrightnessLuma *luma;
	PLUGIN_GUI_CLASS_MEMBERS
};

class BrightnessSlider : public BC_FSlider
{
public:
	BrightnessSlider(BrightnessMain *client, double *output, int x, int y, int is_brightness);

	int handle_event();
	char* get_caption();

	BrightnessMain *client;
	double *output;
	int is_brightness;
	char string[64];
};

class BrightnessLuma : public BC_CheckBox
{
public:
	BrightnessLuma(BrightnessMain *client, int x, int y);

	int handle_event();

	BrightnessMain *client;
};

#endif

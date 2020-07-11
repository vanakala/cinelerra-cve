// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef SHARPENWINDOW_H
#define SHARPENWINDOW_H

class SharpenInterlace;

#include "bcslider.h"
#include "bctoggle.h"
#include "filexml.h"
#include "mutex.h"
#include "sharpen.h"
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER

class SharpenSlider;
class SharpenHorizontal;
class SharpenLuminance;

class SharpenWindow : public PluginWindow
{
public:
	SharpenWindow(SharpenMain *plugin, int x, int y);

	void update();

	SharpenSlider *sharpen_slider;
	SharpenInterlace *sharpen_interlace;
	SharpenHorizontal *sharpen_horizontal;
	SharpenLuminance *sharpen_luminance;
	PLUGIN_GUI_CLASS_MEMBERS
};

class SharpenSlider : public BC_ISlider
{
public:
	SharpenSlider(SharpenMain *client, double *output, int x, int y);

	int handle_event();

	SharpenMain *client;
	double *output;
};

class SharpenInterlace : public BC_CheckBox
{
public:
	SharpenInterlace(SharpenMain *client, int x, int y);

	int handle_event();

	SharpenMain *client;
};

class SharpenHorizontal : public BC_CheckBox
{
public:
	SharpenHorizontal(SharpenMain *client, int x, int y);

	int handle_event();

	SharpenMain *client;
};

class SharpenLuminance : public BC_CheckBox
{
public:
	SharpenLuminance(SharpenMain *client, int x, int y);

	int handle_event();

	SharpenMain *client;
};

#endif

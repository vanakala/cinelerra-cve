// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BLURWINDOW_H
#define BLURWINDOW_H

#include "bctoggle.h"
#include "bcpot.h"
#include "blur.h"
#include "mutex.h"
#include "pluginwindow.h"

class BlurRadius;

PLUGIN_THREAD_HEADER

class BlurCheckBox : public BC_CheckBox
{
public:
	BlurCheckBox(BlurMain *plugin, int x, int y, int *value,
		const char *boxname);

	int handle_event();
private:
	BlurMain *plugin;
	int *value;
};

class BlurWindow : public PluginWindow
{
public:
	BlurWindow(BlurMain *plugin, int x, int y);

	void update();

	BlurCheckBox *vertical;
	BlurCheckBox *horizontal;
	BlurRadius *radius;
	BlurCheckBox *chan0;
	BlurCheckBox *chan1;
	BlurCheckBox *chan2;
	BlurCheckBox *chan3;
	PLUGIN_GUI_CLASS_MEMBERS
private:
	static const char *blur_chn_rgba[];
	static const char *blur_chn_ayuv[];
};

class BlurRadius : public BC_IPot
{
public:
	BlurRadius(BlurMain *client, int x, int y);

	int handle_event();

	BlurMain *client;
};

#endif

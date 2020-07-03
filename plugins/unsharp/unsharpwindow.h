// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef UNSHARPWINDOW_H
#define UNSHARPWINDOW_H

#include "bcpot.h"
#include "unsharp.h"
#include "pluginwindow.h"

class UnsharpRadius : public BC_FPot
{
public:
	UnsharpRadius(UnsharpMain *plugin, int x, int y);

	int handle_event();
	UnsharpMain *plugin;
};

class UnsharpAmount : public BC_FPot
{
public:
	UnsharpAmount(UnsharpMain *plugin, int x, int y);

	int handle_event();
	UnsharpMain *plugin;
};

class UnsharpThreshold : public BC_IPot
{
public:
	UnsharpThreshold(UnsharpMain *plugin, int x, int y);

	int handle_event();
	UnsharpMain *plugin;
};

class UnsharpWindow : public PluginWindow
{
public:
	UnsharpWindow(UnsharpMain *plugin, int x, int y);

	void update();

	UnsharpRadius *radius;
	UnsharpAmount *amount;
	UnsharpThreshold *threshold;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

#endif

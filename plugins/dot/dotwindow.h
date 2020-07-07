// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef DOTWINDOW_H
#define DOTWINDOW_H

#include "dot.h"
#include "bcslider.h"
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER

class DotSize : public BC_ISlider
{
public:
	DotSize(DotMain *plugin, int x, int y,
		int *output, int min, int max);

	int handle_event();
private:
	DotMain *plugin;
	int *output;
};

class DotWindow : public PluginWindow
{
public:
	DotWindow(DotMain *plugin, int x, int y);

	void update();

	DotSize *dot_depth;
	DotSize *dot_size;

	PLUGIN_GUI_CLASS_MEMBERS
};

#endif

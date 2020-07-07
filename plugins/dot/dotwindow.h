// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef DOTWINDOW_H
#define DOTWINDOW_H

#include "dot.h"
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER

class DotWindow : public PluginWindow
{
public:
	DotWindow(DotMain *plugin, int x, int y);

	void update();

	PLUGIN_GUI_CLASS_MEMBERS
};

#endif

// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BURNWINDOW_H
#define BURNWINDOW_H

#include "burn.h"
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER

class BurnWindow : public PluginWindow
{
public:
	BurnWindow(BurnMain *plugin, int x, int y);

	void update();
	PLUGIN_GUI_CLASS_MEMBERS
};

#endif

// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef HOLOWINDOW_H
#define HOLOWINDOW_H

#include "holo.h"
#include "pluginclient.h"
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER

class HoloWindow : public PluginWindow
{
public:
	HoloWindow(HoloMain *plugin, int x, int y);

	void update();

	PLUGIN_GUI_CLASS_MEMBERS
};

#endif

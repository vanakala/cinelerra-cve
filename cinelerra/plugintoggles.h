// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PLUGINTOGGLES_H
#define PLUGINTOGGLES_H

#include "plugin.inc"


class PluginOn : public BC_Toggle
{
public:
	PluginOn(int x, int y, Plugin *plugin);

	static int calculate_w();
	void update(int x, int y, Plugin *plugin);
	int handle_event();

	int in_use;
	Plugin *plugin;
};


class PluginShow : public BC_Toggle
{
public:
	PluginShow(int x, int y, Plugin *plugin);

	static int calculate_w();
	void update(int x, int y, Plugin *plugin);
	int handle_event();

	int in_use;
	Plugin *plugin;
};

#endif

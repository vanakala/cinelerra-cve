// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef TRACKPLUGIN_H
#define TRACKPLUGIN_H

#include "bcsubwindow.h"
#include "bctoggle.h"
#include "trackplugin.inc"
#include "plugin.inc"

class TrackPlugin : public BC_SubWindow
{
public:
	TrackPlugin(int x, int y, int w, int h, Plugin *plugin);
	~TrackPlugin();

	void show();
	void update(int x, int y, int w, int h);
	void update_toggles();
private:
	void redraw(int x, int y, int w, int h);

	int drawn_x;
	int drawn_y;
	int drawn_w;
	int drawn_h;

	Plugin *plugin;
	PluginOn *plugin_on;
	PluginShow *plugin_show;
	BC_Pixmap *background;
};

class PluginOn : public BC_Toggle
{
public:
	PluginOn(int x, Plugin *plugin);

	static int calculate_w();
	void update(int x);
	int handle_event();
private:
	Plugin *plugin;
};


class PluginShow : public BC_Toggle
{
public:
	PluginShow(int x, Plugin *plugin);

	static int calculate_w();
	void update(int x);
	int handle_event();
private:
	Plugin *plugin;
};

#endif

// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef TRACKPLUGIN_H
#define TRACKPLUGIN_H

#include "bcsubwindow.h"
#include "bctoggle.h"
#include "trackcanvas.inc"
#include "trackplugin.inc"
#include "plugin.inc"

class TrackPlugin : public BC_SubWindow
{
public:
	TrackPlugin(int x, int y, int w, int h,
		Plugin *plugin, TrackCanvas *canvas);
	~TrackPlugin();

	void show();
	void update(int x, int y, int w, int h);
	void update_toggles();
	int cursor_motion_event();
	int button_press_event();
	int drag_start_event();
	void drag_motion_event();
	void drag_stop_event();
private:
	void redraw(int x, int y, int w, int h);
	void draw_keyframe_box(int x);

	int drawn_x;
	int drawn_y;
	int drawn_w;
	int drawn_h;
	int num_keyframes;
	int keyframe_width;
	int drag_box;

	TrackCanvas *canvas;
	Plugin *plugin;
	PluginOn *plugin_on;
	PluginShow *plugin_show;
	BC_Pixmap *keyframe_pixmap;
};

class PluginOn : public BC_Toggle
{
public:
	PluginOn(int x, Plugin *plugin);

	static int calculate_w();
	void update();
	int handle_event();
private:
	Plugin *plugin;
};


class PluginShow : public BC_Toggle
{
public:
	PluginShow(int x, Plugin *plugin);

	static int calculate_w();
	void update();
	int handle_event();
private:
	Plugin *plugin;
};

#endif

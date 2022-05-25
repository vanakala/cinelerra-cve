// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2022 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef TESTVPLUGIN_H
#define TESTVPLUGIN_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Video Tests")
#define PLUGIN_CLASS TestVPlugin
#define PLUGIN_CONFIG_CLASS TestVPluginConfig
#define PLUGIN_THREAD_CLASS TestVPluginThread
#define PLUGIN_GUI_CLASS TestVPluginWindow
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

#include "pluginmacros.h"

#include "bctoggle.h"
#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

class TestVPluginConfig
{
public:
	TestVPluginConfig();

	int equivalent(TestVPluginConfig &that);
	void copy_from(TestVPluginConfig &that);
	void interpolate(TestVPluginConfig &prev, TestVPluginConfig &next,
		ptstime prev_pts, ptstime next_pts, ptstime current_pts);

	int testguides;
	int color;
	int colorspace;
	int avlibs;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class TestVPluginGuides : public BC_CheckBox
{
public:
	TestVPluginGuides(int x, int y, TestVPlugin *plugin,
		int valuebit, const char *name);

	int handle_event();
	void update(int val);

	TestVPlugin *plugin;
	int valuebit;
};

class TestVPluginColor : public BC_CheckBox
{
public:
	TestVPluginColor(int x, int y, TestVPlugin *plugin,
		int color, const char *name);

	int handle_event();
	void update(int val);

	TestVPlugin *plugin;
	int color;
};

class TestVPluginValue : public BC_CheckBox
{
public:
	TestVPluginValue(int x, int y, TestVPlugin *plugin,
		int *value, const char *name);

	int handle_event();

	TestVPlugin *plugin;
	int *value;
};


class TestVPluginWindow : public PluginWindow
{
public:
	TestVPluginWindow(TestVPlugin *plugin, int x, int y);

	void update();

	TestVPluginGuides *testguide_line;
	TestVPluginGuides *testguide_rect;
	TestVPluginGuides *testguide_box;
	TestVPluginGuides *testguide_disc;
	TestVPluginGuides *testguide_circ;
	TestVPluginGuides *testguide_pixel;
	TestVPluginGuides *testguide_frame;
	TestVPluginColor *testguide_red;
	TestVPluginColor *testguide_green;
	TestVPluginColor *testguide_blue;
	TestVPluginGuides *testguide_blink;
	TestVPluginValue *testcolor;
	TestVPluginValue *testavlibs;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class TestVPlugin : public PluginVClient
{
public:
	TestVPlugin(PluginServer *server);
	~TestVPlugin();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *frame);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
};

#endif

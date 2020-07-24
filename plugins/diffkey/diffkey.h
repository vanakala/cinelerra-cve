// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef DIFFKEY_H
#define DIFFKEY_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_IS_MULTICHANNEL
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_MAX_CHANNELS 2

#define PLUGIN_TITLE N_("Difference key")
#define PLUGIN_CLASS DiffKey
#define PLUGIN_CONFIG_CLASS DiffKeyConfig
#define PLUGIN_THREAD_CLASS DiffKeyThread
#define PLUGIN_GUI_CLASS DiffKeyGUI

#define GL_GLEXT_PROTOTYPES

#include "datatype.h"
#include "language.h"
#include "loadbalance.h"
#include "pluginmacros.h"
#include "pluginserver.inc"
#include "pluginwindow.h"
#include "pluginvclient.h"
#include "vframe.inc"

class DiffKey;

class DiffKeyConfig
{
public:
	DiffKeyConfig();
	void copy_from(DiffKeyConfig &src);
	int equivalent(DiffKeyConfig &src);
	void interpolate(DiffKeyConfig &prev, 
		DiffKeyConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	double threshold;
	double slope;
	int do_value;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class DiffKeyThreshold : public BC_FSlider
{
public:
	DiffKeyThreshold(DiffKey *plugin, int x, int y);
	int handle_event();
	DiffKey *plugin;
};

class DiffKeySlope : public BC_FSlider
{
public:
	DiffKeySlope(DiffKey *plugin, int x, int y);
	int handle_event();
	DiffKey *plugin;
};

class DiffKeyDoValue : public BC_CheckBox
{
public:
	DiffKeyDoValue(DiffKey *plugin, int x, int y);
	int handle_event();
	DiffKey *plugin;
};


class DiffKeyGUI : public PluginWindow
{
public:
	DiffKeyGUI(DiffKey *plugin, int x, int y);

	void update();

	DiffKeyThreshold *threshold;
	DiffKeySlope *slope;
	DiffKeyDoValue *do_value;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class DiffKeyEngine : public LoadServer
{
public:
	DiffKeyEngine(DiffKey *plugin);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	DiffKey *plugin;
};


class DiffKeyClient : public LoadClient
{
public:
	DiffKeyClient(DiffKeyEngine *engine);

	void process_package(LoadPackage *pkg);
	DiffKeyEngine *engine;
};

class DiffKeyPackage : public LoadPackage
{
public:
	DiffKeyPackage();
	int row1;
	int row2;
};


class DiffKey : public PluginVClient
{
public:
	DiffKey(PluginServer *server);
	~DiffKey();

	void process_tmpframes(VFrame **frame);
	void reset_plugin();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void handle_opengl();

	PLUGIN_CLASS_MEMBERS

	DiffKeyEngine *engine;
	VFrame *top_frame;
	VFrame *bottom_frame;
};

#endif

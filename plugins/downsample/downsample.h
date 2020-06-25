// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef DOWNSAMPLE_H
#define DOWNSAMPLE_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Downsample")
#define PLUGIN_CLASS DownSampleMain
#define PLUGIN_CONFIG_CLASS DownSampleConfig
#define PLUGIN_THREAD_CLASS DownSampleThread
#define PLUGIN_GUI_CLASS DownSampleWindow

#include "pluginmacros.h"

#include "bctoggle.h"
#include "language.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

class DownSampleServer;

class DownSampleConfig
{
public:
	DownSampleConfig();

	int equivalent(DownSampleConfig &that);
	void copy_from(DownSampleConfig &that);
	void interpolate(DownSampleConfig &prev, 
		DownSampleConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int horizontal_x;
	int vertical_y;
	int horizontal;
	int vertical;
	int chan0;
	int chan1;
	int chan2;
	int chan3;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class DownSampleToggle : public BC_CheckBox
{
public:
	DownSampleToggle(DownSampleMain *plugin, 
		int x, 
		int y, 
		int *output, 
		char *string);

	int handle_event();
	DownSampleMain *plugin;
	int *output;
};

class DownSampleSize : public BC_ISlider
{
public:
	DownSampleSize(DownSampleMain *plugin, 
		int x, 
		int y, 
		int *output,
		int min,
		int max);

	int handle_event();
	DownSampleMain *plugin;
	int *output;
};

class DownSampleWindow : public PluginWindow
{
public:
	DownSampleWindow(DownSampleMain *plugin, int x, int y);

	void update();

	DownSampleToggle *chan0;
	DownSampleToggle *chan1;
	DownSampleToggle *chan2;
	DownSampleToggle *chan3;
	DownSampleSize *h, *v, *h_x, *v_y;
	PLUGIN_GUI_CLASS_MEMBERS
private:
	static const char *ds_chn_rgba[];
	static const char *ds_chn_ayuv[];
};


PLUGIN_THREAD_HEADER


class DownSampleMain : public PluginVClient
{
public:
	DownSampleMain(PluginServer *server);
	~DownSampleMain();

	VFrame *process_tmpframe(VFrame *input_ptr);
	void reset_plugin();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	PLUGIN_CLASS_MEMBERS

	VFrame *output;
	DownSampleServer *engine;
};

class DownSamplePackage : public LoadPackage
{
public:
	DownSamplePackage();
	int y1, y2;
};

class DownSampleUnit : public LoadClient
{
public:
	DownSampleUnit(DownSampleServer *server, DownSampleMain *plugin);

	void process_package(LoadPackage *package);

	DownSampleServer *server;
	DownSampleMain *plugin;
};

class DownSampleServer : public LoadServer
{
public:
	DownSampleServer(DownSampleMain *plugin, 
		int total_clients, 
		int total_packages);

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	DownSampleMain *plugin;
};

#endif

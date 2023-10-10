// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>

#ifndef COLOR3WAY_H
#define COLOR3WAY_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE  N_("Color 3 Way")
#define PLUGIN_CLASS Color3WayMain
#define PLUGIN_CONFIG_CLASS Color3WayConfig
#define PLUGIN_THREAD_CLASS Color3WayThread
#define PLUGIN_GUI_CLASS Color3WayWindow

#include "pluginmacros.h"

class Color3WayEngine;
class Color3WayThread;

#define SHADOWS 0
#define MIDTONES 1
#define HIGHLIGHTS 2
#define SECTIONS 3

#include "color3waywindow.h"
#include "pluginvclient.h"
#include "language.h"
#include "loadbalance.h"
#include "thread.h"

class Color3WayConfig
{
public:
	Color3WayConfig();

	int equivalent(Color3WayConfig &that);
	void copy_from(Color3WayConfig &that);
	void interpolate(Color3WayConfig &prev,
		Color3WayConfig &next,
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);
	void boundaries();
	void copy2all(int section);

	double hue_x[SECTIONS];
	double hue_y[SECTIONS];
	double value[SECTIONS];
	double saturation[SECTIONS];
	int copy_to_all[SECTIONS];
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class Color3WayPackage : public LoadPackage
{
public:
	Color3WayPackage();

	int row1, row2;
};


class Color3WayUnit : public LoadClient
{
public:
	Color3WayUnit(Color3WayMain *plugin, Color3WayEngine *server);

	void process_package(LoadPackage *package);
	void level_coefs(double value, double *coefs);
	void level_coefs(int value, int *coefs);

	Color3WayMain *plugin;
};

class Color3WayEngine : public LoadServer
{
public:
	Color3WayEngine(Color3WayMain *plugin, int cpus);

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	Color3WayMain *plugin;
};

class Color3WayMain : public PluginVClient
{
public:
	Color3WayMain(PluginServer *server);
	~Color3WayMain();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *frame);
	void reset_plugin();

	void load_defaults();
	void save_defaults();

	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	void get_aggregation(int *aggregate_gamma);

	void calculate_rgb(int64_t *red, int64_t *green, int64_t *blue,
		double *sat, double *val, int section);

	Color3WayEngine *engine;
	VFrame *input;

	double color_max;
};

#endif

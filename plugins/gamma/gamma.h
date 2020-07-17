// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef LINEARIZE_H
#define LINEARIZE_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_STATUS_GUI

#define PLUGIN_TITLE N_("Gamma")
#define PLUGIN_CLASS GammaMain
#define PLUGIN_CONFIG_CLASS GammaConfig
#define PLUGIN_THREAD_CLASS GammaThread
#define PLUGIN_GUI_CLASS GammaWindow

#include "pluginmacros.h"

class GammaEngine;

#include "gammawindow.h"
#include "loadbalance.h"
#include "language.h"
#include "pluginvclient.h"

#define HISTOGRAM_SIZE 256

class GammaConfig
{
public:
	GammaConfig();

	int equivalent(GammaConfig &that);
	void copy_from(GammaConfig &that);
	void interpolate(GammaConfig &prev, 
		GammaConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	double max;
	double gamma;
	int automatic;
// Plot histogram
	int plot;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class GammaPackage : public LoadPackage
{
public:
	GammaPackage();
	int start, end;
};

class GammaUnit : public LoadClient
{
public:
	GammaUnit(GammaMain *plugin);
	void process_package(LoadPackage *package);
	GammaMain *plugin;
	int accum[HISTOGRAM_SIZE];
};

class GammaEngine : public LoadServer
{
public:
	GammaEngine(GammaMain *plugin);

	void process_packages(int operation, VFrame *data);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	VFrame *data;
	int operation;
	enum
	{
		HISTOGRAM,
		APPLY
	};
	GammaMain *plugin;
	int accum[HISTOGRAM_SIZE];
};

class GammaMain : public PluginVClient
{
public:
	GammaMain(PluginServer *server);
	~GammaMain();

	VFrame *process_tmpframe(VFrame *frame);
	void reset_plugin();
	void calculate_max(VFrame *frame);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();
	void render_gui(void *data);
	void handle_opengl();

	GammaEngine *engine;

	PLUGIN_CLASS_MEMBERS
};

#endif

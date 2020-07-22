// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_STATUS_GUI

#define PLUGIN_TITLE N_("Histogram")
#define PLUGIN_CLASS HistogramMain
#define PLUGIN_CONFIG_CLASS HistogramConfig
#define PLUGIN_THREAD_CLASS HistogramThread
#define PLUGIN_GUI_CLASS HistogramWindow

#include "pluginmacros.h"

#include "histogram.inc"
#include "histogramconfig.h"
#include "language.h"
#include "loadbalance.h"
#include "pluginvclient.h"

class HistogramMain : public PluginVClient
{
public:
	HistogramMain(PluginServer *server);
	~HistogramMain();

	VFrame *process_tmpframe(VFrame *frame);
	void reset_plugin();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void handle_opengl();

	PLUGIN_CLASS_MEMBERS

// Interpolate quantized transfer table to linear output
	double calculate_linear(double input, int mode);
	double calculate_smooth(double input, int subscript);
// Calculate automatic settings
	void calculate_automatic(VFrame *data);
// Calculate histogram.
	void calculate_histogram(VFrame *data);
// Calculate the linear, lookup curves
	void tabulate_curve(int subscript);

	VFrame *input;
	HistogramEngine *engine;
	int *lookup[HISTOGRAM_MODES];
	float *linear[HISTOGRAM_MODES];
	int *accum[HISTOGRAM_MODES];
// Current channel being viewed
	int mode;
};

class HistogramPackage : public LoadPackage
{
public:
	HistogramPackage();
	int start, end;
	int package_done;
};

class HistogramUnit : public LoadClient
{
public:
	HistogramUnit(HistogramEngine *server, HistogramMain *plugin);
	~HistogramUnit();

	void process_package(LoadPackage *package);

	HistogramEngine *server;
	HistogramMain *plugin;
	int *accum[HISTOGRAM_MODES];
	int package_done;
};

class HistogramEngine : public LoadServer
{
public:
	HistogramEngine(HistogramMain *plugin, 
		int total_clients, 
		int total_packages);
	void process_packages(int operation, VFrame *data);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	HistogramMain *plugin;
	int total_size;

	int operation;
	enum
	{
		HISTOGRAM,
		APPLY
	};
	VFrame *data;
};

#endif

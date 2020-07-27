// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef THRESHOLD_H
#define THRESHOLD_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_STATUS_GUI

#define PLUGIN_TITLE N_("Threshold")
#define PLUGIN_CLASS ThresholdMain
#define PLUGIN_CONFIG_CLASS ThresholdConfig
#define PLUGIN_THREAD_CLASS ThresholdThread
#define PLUGIN_GUI_CLASS ThresholdWindow

#include "pluginmacros.h"

#include "histogramengine.inc"
#include "language.h"
#include "loadbalance.h"
#include "pluginvclient.h"


class ThresholdEngine;
class RGBA;

// Color components are in range [0, 0xffff].
class RGBA
{
public:
	RGBA();   // Initialize to transparent black.
	RGBA(int r, int g, int b, int a);

	void set(int r, int g, int b, int a);
	int getRGB() const;

	int  r, g, b, a;
};

bool operator==(const RGBA & a, const RGBA & b);

// General purpose scale function.
template<typename T>
T interpolate(const T & prev, const double & prev_scale, const T & next, const double & next_scale);

// Specialization for RGBA class.
template<>
RGBA interpolate(const RGBA & prev_color, const double & prev_scale, const RGBA &next_color, const double & next_scale);


class ThresholdConfig
{
public:
	ThresholdConfig();
	int equivalent(ThresholdConfig &that);
	void copy_from(ThresholdConfig &that);
	void interpolate(ThresholdConfig &prev,
		ThresholdConfig &next,
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);
	void boundaries();

	double min;
	double max;
	int plot;
	RGBA low_color;
	RGBA mid_color;
	RGBA high_color;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class ThresholdMain : public PluginVClient
{
public:
	ThresholdMain(PluginServer *server);
	~ThresholdMain();

	VFrame *process_tmpframe(VFrame *frame);
	void reset_plugin();
	void load_defaults();
	int adjusted_default(BC_Hash *defaults, const char *oldkey,
		const char *newkey, int default_value);
	int adjusted_property(FileXML *file, const char *oldkey,
		const char *newkey, int default_value);
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void calculate_histogram(VFrame *frame);
	void handle_opengl();

	PLUGIN_CLASS_MEMBERS
	HistogramEngine *engine;
	ThresholdEngine *threshold_engine;
};


class ThresholdPackage : public LoadPackage
{
public:
	ThresholdPackage();
	int start;
	int end;
};


class ThresholdUnit : public LoadClient
{
public:
	ThresholdUnit(ThresholdEngine *server);
	void process_package(LoadPackage *package);

	ThresholdEngine *server;
};


class ThresholdEngine : public LoadServer
{
public:
	ThresholdEngine(ThresholdMain *plugin);

	void process_packages(VFrame *data);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	ThresholdMain *plugin;
	VFrame *data;
};

#endif

// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef HUESATURATION_H
#define HUESATURATION_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Hue saturation")
#define PLUGIN_CLASS HueEffect
#define PLUGIN_CONFIG_CLASS HueConfig
#define PLUGIN_THREAD_CLASS HueThread
#define PLUGIN_GUI_CLASS HueWindow

#include "pluginmacros.h"

#include "bcslider.h"
#include "language.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

#define MINHUE -180
#define MAXHUE 180
#define MINSATURATION -100
#define MAXSATURATION 100
#define MINVALUE -100
#define MAXVALUE 100


class HueConfig
{
public:
	HueConfig();

	void copy_from(HueConfig &src);
	int equivalent(HueConfig &src);
	void interpolate(HueConfig &prev, 
		HueConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);
	int hue;
	double saturation;
	double value;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class HueSlider : public BC_ISlider
{
public:
	HueSlider(HueEffect *plugin, int x, int y, int w);

	int handle_event();
	HueEffect *plugin;
};

class SaturationSlider : public BC_FSlider
{
public:
	SaturationSlider(HueEffect *plugin, int x, int y, int w);

	int handle_event();
	char* get_caption();

	HueEffect *plugin;
	char string[64];
};

class ValueSlider : public BC_FSlider
{
public:
	ValueSlider(HueEffect *plugin, int x, int y, int w);

	int handle_event();
	char* get_caption();

	HueEffect *plugin;
	char string[64];
};

class HueWindow : public PluginWindow
{
public:
	HueWindow(HueEffect *plugin, int x, int y);

	void update();

	HueSlider *hue;
	SaturationSlider *saturation;
	ValueSlider *value;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class HueEngine : public LoadServer
{
public:
	HueEngine(HueEffect *plugin, int cpus);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	HueEffect *plugin;
};

class HuePackage : public LoadPackage
{
public:
	HuePackage();
	int row1, row2;
};

class HueUnit : public LoadClient
{
public:
	HueUnit(HueEffect *plugin, HueEngine *server);

	void process_package(LoadPackage *package);
	HueEffect *plugin;
};

class HueEffect : public PluginVClient
{
public:
	HueEffect(PluginServer *server);
	~HueEffect();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *frame);
	void reset_plugin();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void handle_opengl();

	VFrame *input;
	HueEngine *engine;
};

#endif

// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef WAVE_H
#define WAVE_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Wave")
#define PLUGIN_CLASS WaveEffect
#define PLUGIN_CONFIG_CLASS WaveConfig
#define PLUGIN_THREAD_CLASS WaveThread
#define PLUGIN_GUI_CLASS WaveWindow

#include "pluginmacros.h"

#include "bcslider.h"
#include "keyframe.inc"
#include "language.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

#define SMEAR 0
#define BLACKEN 1

class WaveConfig
{
public:
	WaveConfig();
	void copy_from(WaveConfig &src);
	int equivalent(WaveConfig &src);
	void interpolate(WaveConfig &prev, 
		WaveConfig &next,
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);
	int mode;
	int reflective;
	double amplitude;
	double phase;
	double wavelength;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class WaveAmplitude : public BC_FSlider
{
public:
	WaveAmplitude(WaveEffect *plugin, int x, int y);
	int handle_event();
	WaveEffect *plugin;
};

class WavePhase : public BC_FSlider
{
public:
	WavePhase(WaveEffect *plugin, int x, int y);
	int handle_event();
	WaveEffect *plugin;
};

class WaveLength : public BC_FSlider
{
public:
	WaveLength(WaveEffect *plugin, int x, int y);
	int handle_event();
	WaveEffect *plugin;
};

class WaveWindow : public PluginWindow
{
public:
	WaveWindow(WaveEffect *plugin, int x, int y);

	void update();
	WaveEffect *plugin;
	WaveAmplitude *amplitude;
	WavePhase *phase;
	WaveLength *wavelength;
};


PLUGIN_THREAD_HEADER


class WaveServer : public LoadServer
{
public:
	WaveServer(WaveEffect *plugin, int cpus);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	WaveEffect *plugin;
};

class WavePackage : public LoadPackage
{
public:
	WavePackage();
	int row1, row2;
};

class WaveUnit : public LoadClient
{
public:
	WaveUnit(WaveEffect *plugin, WaveServer *server);
	void process_package(LoadPackage *package);
	WaveEffect *plugin;
};


class WaveEffect : public PluginVClient
{
public:
	WaveEffect(PluginServer *server);
	~WaveEffect();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *input);
	void reset_plugin();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	VFrame *input, *output;
	WaveServer *engine;
};

#endif

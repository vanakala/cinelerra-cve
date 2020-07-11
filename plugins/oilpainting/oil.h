// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef OIL_H
#define OIL_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Oil painting")
#define PLUGIN_CLASS OilEffect
#define PLUGIN_CONFIG_CLASS OilConfig
#define PLUGIN_THREAD_CLASS OilThread
#define PLUGIN_GUI_CLASS OilWindow

#include "pluginmacros.h"

#include "bctoggle.h"
#include "bcslider.h"
#include "keyframe.inc"
#include "language.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"


// Algorithm by Torsten Martinsen
// Ported to Cinelerra by Heroine Virtual Ltd.


class OilConfig
{
public:
	OilConfig();

	void copy_from(OilConfig &src);
	int equivalent(OilConfig &src);
	void interpolate(OilConfig &prev, 
		OilConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	double radius;
	int use_intensity;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class OilRadius : public BC_FSlider
{
public:
	OilRadius(OilEffect *plugin, int x, int y);

	int handle_event();
	OilEffect *plugin;
};


class OilIntensity : public BC_CheckBox
{
public:
	OilIntensity(OilEffect *plugin, int x, int y);

	int handle_event();
	OilEffect *plugin;
};

class OilWindow : public PluginWindow
{
public:
	OilWindow(OilEffect *plugin, int x, int y);

	void update();

	OilRadius *radius;
	OilIntensity *intensity;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class OilServer : public LoadServer
{
public:
	OilServer(OilEffect *plugin, int cpus);

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	OilEffect *plugin;
};

class OilPackage : public LoadPackage
{
public:
	OilPackage();
	int row1, row2;
};

class OilUnit : public LoadClient
{
public:
	OilUnit(OilEffect *plugin, OilServer *server);
	~OilUnit();

	void process_package(LoadPackage *package);

	OilEffect *plugin;
	int *hist[4];
	int *hist2;
};


class OilEffect : public PluginVClient
{
public:
	OilEffect(PluginServer *server);
	~OilEffect();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *input);
	void reset_plugin();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	VFrame *input, *output;
	OilServer *engine;
};

#endif

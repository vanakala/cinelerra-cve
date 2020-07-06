// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BURN_H
#define BURN_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("BurningTV")
#define PLUGIN_CLASS BurnMain
#define PLUGIN_CONFIG_CLASS BurnConfig
#define PLUGIN_THREAD_CLASS BurnThread
#define PLUGIN_GUI_CLASS BurnWindow

#include "pluginmacros.h"

#include "effecttv.inc"
#include "language.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "burnwindow.h"

class BurnConfig
{
public:
	BurnConfig();

	int equivalent(BurnConfig &that);
	void copy_from(BurnConfig &that);
	void interpolate(BurnConfig &prev,
		BurnConfig &next,
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int threshold;
	int decay;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class BurnPackage : public LoadPackage
{
public:
	BurnPackage();

	int row1, row2;
};

class BurnServer : public LoadServer
{
public:
	BurnServer(BurnMain *plugin, int total_clients, int total_packages);

	LoadClient* new_client();
	LoadPackage* new_package();
	void init_packages();

	BurnMain *plugin;
};

class BurnClient : public LoadClient
{
public:
	BurnClient(BurnServer *server);

	void process_package(LoadPackage *package);

	BurnMain *plugin;
};


class BurnMain : public PluginVClient
{
public:
	BurnMain(PluginServer *server);
	~BurnMain();

	PLUGIN_CLASS_MEMBERS

// required for all realtime plugins
	VFrame *process_tmpframe(VFrame *input);
	void reset_plugin();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void HSItoRGB(double H, 
		double S, 
		double I, 
		int *r, 
		int *g, 
		int *b,
		int color_model);
	void make_palette(int color_model);

	BurnServer *burn_server;

	int palette[3][256];
	unsigned char *buffer;

	int total;

	EffectTV *effecttv;
	VFrame *input_ptr;
};

#endif

// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef DOT_H
#define DOT_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("DotTV")
#define PLUGIN_CLASS DotMain
#define PLUGIN_CONFIG_CLASS DotConfig
#define PLUGIN_THREAD_CLASS DotThread
#define PLUGIN_GUI_CLASS DotWindow

#include "pluginmacros.h"

class DotEngine;

#include "bchash.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "dotwindow.h"
#include "language.h"

#include <stdint.h>


class DotConfig
{
public:
	DotConfig();

	int equivalent(DotConfig &that);
	void copy_from(DotConfig &that);
	void interpolate(DotConfig &prev,
		DotConfig &next,
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int dot_depth;
	int dot_size;
	inline int dot_max()
	{
		return 1 << dot_depth;
	};
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class DotPackage : public LoadPackage
{
public:
	DotPackage();

	int row1, row2;
};

class DotServer : public LoadServer
{
public:
	DotServer(DotMain *plugin, int total_clients, int total_packages);

	LoadClient* new_client();
	LoadPackage* new_package();
	void init_packages();
	DotMain *plugin;
};

class DotClient : public LoadClient
{
public:
	DotClient(DotServer *server);

	void process_package(LoadPackage *package);
	void draw_dot(int xx, int yy,
		int c, int first_row);

	int RGBtoY(uint16_t *row, int color_model);

	DotMain *plugin;
};


class DotMain : public PluginVClient
{
public:
	DotMain(PluginServer *server);
	~DotMain();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *input_ptr);
	void reset_plugin();
	void make_pattern();
	void init_sampxy_table();
	void reconfigure();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	DotServer *dot_server;
	DotClient *dot_client;

	VFrame *input_ptr, *output_ptr;
	int dots_width;
	int dots_height;
	int dot_size;
	int dot_hsize;
	uint32_t *pattern;
	int pattern_allocated;
	int pattern_size;
	int *sampx, *sampy;
};

#endif

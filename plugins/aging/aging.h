// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef AGING_H
#define AGING_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_TITLE N_("AgingTV")
#define PLUGIN_CLASS AgingMain
#define PLUGIN_CONFIG_CLASS AgingConfig
#define PLUGIN_THREAD_CLASS AgingThread
#define PLUGIN_GUI_CLASS AgingWindow

#include "pluginmacros.h"

class AgingEngine;

#include "bchash.h"
#include "keyframe.inc"
#include "language.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "agingwindow.h"

#include <sys/types.h>

#define SCRATCH_MAX 20
#define AREA_SCALE_MAX 40
#define DUST_INTERVAL_MAX 80

typedef struct _scratch
{
	int life;
	int x;
	int dx;
	int init;
} scratch_t;

class AgingConfig
{
public:
	AgingConfig();

	int equivalent(AgingConfig &that);
	void copy_from(AgingConfig &that);
	void interpolate(AgingConfig &prev,
		AgingConfig &next,
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int area_scale;
	int scratch_lines;
	int dust_interval;
	int colorage;
	int scratch;
	int pits;
	int dust;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class AgingPackage : public LoadPackage
{
public:
	AgingPackage();

	int row1, row2;
};

class AgingServer : public LoadServer
{
public:
	AgingServer(AgingMain *plugin, int total_clients, int total_packages);

	LoadClient* new_client();
	LoadPackage* new_package();
	void init_packages();
	AgingMain *plugin;
};

class AgingClient : public LoadClient
{
public:
	AgingClient(AgingServer *server);

	void coloraging(VFrame *output_frame,
		int row1, int row2);
	void scratching(VFrame *output_frame,
		int row1, int row2);
	void pits(VFrame *output_frame,
		int row1, int row2);
	void dusts(VFrame *output_frame,
		int row1, int row2);
	void process_package(LoadPackage *package);

	AgingMain *plugin;
};


class AgingMain : public PluginVClient
{
public:
	AgingMain(PluginServer *server);
	~AgingMain();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *input_ptr);
	void reset_plugin();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	AgingServer *aging_server;
	AgingClient *aging_client;

	AgingEngine **engine;
	VFrame *input;
	scratch_t scratches[SCRATCH_MAX];
	static int dx[8];
	static int dy[8];
	int dust_interval;
	int pits_interval;
};

#endif

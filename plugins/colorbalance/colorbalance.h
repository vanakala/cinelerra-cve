// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef COLORBALANCE_H
#define COLORBALANCE_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE  N_("Color Balance")
#define PLUGIN_CLASS ColorBalanceMain
#define PLUGIN_CONFIG_CLASS ColorBalanceConfig
#define PLUGIN_THREAD_CLASS ColorBalanceThread
#define PLUGIN_GUI_CLASS ColorBalanceWindow

#include "pluginmacros.h"
#include "condition.h"

class ColorBalanceMain;

#include "colorbalancewindow.h"
#include "pluginvclient.h"
#include "language.h"
#include "thread.h"

#define CB_MAX_ENGINES 2

class ColorBalanceConfig
{
public:
	ColorBalanceConfig();

	int equivalent(ColorBalanceConfig &that);
	void copy_from(ColorBalanceConfig &that);
	void interpolate(ColorBalanceConfig &prev, 
		ColorBalanceConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

// -1000 - 1000
	double cyan;
	double magenta;
	double yellow;
	int preserve;
	int lock_params;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class ColorBalanceEngine : public Thread
{
public:
	ColorBalanceEngine(ColorBalanceMain *plugin);
	~ColorBalanceEngine();

	void start_process_frame(VFrame *input, int row_start, int row_end);
	void wait_process_frame();
	void run();

	ColorBalanceMain *plugin;
	int row_start, row_end;
	int last_frame;
	Condition input_lock, output_lock;
	VFrame *input;
};

class ColorBalanceMain : public PluginVClient
{
public:
	ColorBalanceMain(PluginServer *server);
	~ColorBalanceMain();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *frame);
	void reset_plugin();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();
	void handle_opengl();

	double calculate_slider(double in);
	double calculate_transfer(double in);

// parameters needed for processor
	void reconfigure();
	void synchronize_params(ColorBalanceSlider *slider, double difference);
	void test_boundary(double &value);

	ColorBalanceEngine *engines[CB_MAX_ENGINES];
	int total_engines;

	int r_lookup_16[0x10000];
	int g_lookup_16[0x10000];
	int b_lookup_16[0x10000];
};

#endif

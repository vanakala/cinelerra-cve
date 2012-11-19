
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef COLORBALANCE_H
#define COLORBALANCE_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME

#define PLUGIN_TITLE  N_("Color Balance")
#define PLUGIN_CLASS ColorBalanceMain
#define PLUGIN_CONFIG_CLASS ColorBalanceConfig
#define PLUGIN_THREAD_CLASS ColorBalanceThread
#define PLUGIN_GUI_CLASS ColorBalanceWindow

#include "pluginmacros.h"

class ColorBalanceMain;

#include "colorbalancewindow.h"
#include "plugincolors.h"
#include "pluginvclient.h"
#include "language.h"
#include "thread.h"

#define SHADOWS 0
#define MIDTONES 1
#define HIGHLIGHTS 2

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
	float cyan;
	float magenta;
	float yellow;
	int preserve;
	int lock_params;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class ColorBalanceEngine : public Thread
{
public:
	ColorBalanceEngine(ColorBalanceMain *plugin);
	~ColorBalanceEngine();

	void start_process_frame(VFrame *output, VFrame *input, int row_start, int row_end);
	void wait_process_frame();
	void run();

	ColorBalanceMain *plugin;
	int row_start, row_end;
	int last_frame;
	Condition input_lock, output_lock;
	VFrame *input, *output;
	YUV yuv;
	float cyan_f, magenta_f, yellow_f;
};

class ColorBalanceMain : public PluginVClient
{
public:
	ColorBalanceMain(PluginServer *server);
	~ColorBalanceMain();

	PLUGIN_CLASS_MEMBERS

	void process_frame(VFrame *frame);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();
	void handle_opengl();

	void get_aggregation(int *aggregate_gamma);

	float calculate_slider(float in);
	float calculate_transfer(float in);

// parameters needed for processor
	void reconfigure();
	void synchronize_params(ColorBalanceSlider *slider, float difference);
	void test_boundary(float &value);

	ColorBalanceEngine **engine;
	int total_engines;

	int r_lookup_8[0x100];
	int g_lookup_8[0x100];
	int b_lookup_8[0x100];
	int r_lookup_16[0x10000];
	int g_lookup_16[0x10000];
	int b_lookup_16[0x10000];
	int redo_buffers;
	int need_reconfigure;
};

#endif

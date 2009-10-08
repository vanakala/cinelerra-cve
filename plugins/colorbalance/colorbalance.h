
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

class ColorBalanceMain;

#include "colorbalancewindow.h"
#include "condition.h"
#include "plugincolors.h"
#include "guicast.h"
#include "pluginvclient.h"
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
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);

// -1000 - 1000
	float cyan;
	float magenta;
    float yellow;
    int preserve;
    int lock_params;
};

class ColorBalanceEngine : public Thread
{
public:
	ColorBalanceEngine(ColorBalanceMain *plugin);
	~ColorBalanceEngine();

	int start_process_frame(VFrame *output, VFrame *input, int row_start, int row_end);
	int wait_process_frame();
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

// required for all realtime plugins
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	char* plugin_title();
	int show_gui();
	void update_gui();
	void raise_window();
	int set_string();
	int load_configuration();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_defaults();
	int save_defaults();
	VFrame* new_picon();
	int handle_opengl();

	void get_aggregation(int *aggregate_interpolate,
		int *aggregate_gamma);


	int64_t calculate_slider(float in);
	float calculate_transfer(float in);

// parameters needed for processor
	int reconfigure();
    int synchronize_params(ColorBalanceSlider *slider, float difference);
    int test_boundary(float &value);

	ColorBalanceConfig config;
// a thread for the GUI
	ColorBalanceThread *thread;
	ColorBalanceEngine **engine;
	int total_engines;


	BC_Hash *defaults;
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

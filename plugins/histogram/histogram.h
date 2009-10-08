
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

#ifndef HISTOGRAM_H
#define HISTOGRAM_H


#include "histogram.inc"
#include "histogramconfig.h"
#include "histogramwindow.inc"
#include "loadbalance.h"
#include "plugincolors.h"
#include "pluginvclient.h"


class HistogramMain : public PluginVClient
{
public:
	HistogramMain(PluginServer *server);
	~HistogramMain();

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	void render_gui(void *data);
	int calculate_use_opengl();
	int handle_opengl();

	PLUGIN_CLASS_MEMBERS(HistogramConfig, HistogramThread)

// Interpolate quantized transfer table to linear output
	float calculate_linear(float input, int mode, int do_value);
	float calculate_smooth(float input, int subscript);
// Convert input to smoothed output by looking up in smooth table.
	float calculate_curve(float input);
// Calculate automatic settings
	void calculate_automatic(VFrame *data);
// Calculate histogram.
// Value is only calculated for preview.
	void calculate_histogram(VFrame *data, int do_value);
// Calculate the linear, smoothed, lookup curves
	void tabulate_curve(int subscript, int use_value);




	YUV yuv;
	VFrame *input, *output;
	HistogramEngine *engine;
	int *lookup[HISTOGRAM_MODES];
	float *smoothed[HISTOGRAM_MODES];
	float *linear[HISTOGRAM_MODES];
// No value applied to this
	int *preview_lookup[HISTOGRAM_MODES];
	int *accum[HISTOGRAM_MODES];
// Input point being dragged or edited
	int current_point;
// Current channel being viewed
	int mode;
	int dragging_point;
	int point_x_offset;
	int point_y_offset;
};

class HistogramPackage : public LoadPackage
{
public:
	HistogramPackage();
	int start, end;
};

class HistogramUnit : public LoadClient
{
public:
	HistogramUnit(HistogramEngine *server, HistogramMain *plugin);
	~HistogramUnit();
	void process_package(LoadPackage *package);
	HistogramEngine *server;
	HistogramMain *plugin;
	int *accum[5];
};

class HistogramEngine : public LoadServer
{
public:
	HistogramEngine(HistogramMain *plugin, 
		int total_clients, 
		int total_packages);
	void process_packages(int operation, VFrame *data, int do_value);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	HistogramMain *plugin;
	int total_size;


	int operation;
	enum
	{
		HISTOGRAM,
		APPLY
	};
	VFrame *data;
	int do_value;
};











#endif

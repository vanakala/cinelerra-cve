
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

#ifndef TIMEAVG_H
#define TIMEAVG_H

class TimeAvgMain;

#include "bchash.inc"
#include "pluginvclient.h"
#include "timeavgwindow.h"
#include "vframe.inc"

class TimeAvgConfig
{
public:
	TimeAvgConfig();
	void copy_from(TimeAvgConfig *src);
	int equivalent(TimeAvgConfig *src);

	int frames;
	int mode;
	enum
	{
		AVERAGE,
		ACCUMULATE,
		OR
	};
	int paranoid;
	int nosubtract;
};


class TimeAvgMain : public PluginVClient
{
public:
	TimeAvgMain(PluginServer *server);
	~TimeAvgMain();

// required for all realtime plugins
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	char* plugin_title();
	VFrame* new_picon();
	int show_gui();
	int load_configuration();
	int set_string();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void raise_window();
	void update_gui();
	void clear_accum(int w, int h, int color_model);
	void subtract_accum(VFrame *frame);
	void add_accum(VFrame *frame);
	void transfer_accum(VFrame *frame);


	VFrame **history;
// Frame of history in requested samplerate
	int64_t *history_frame;
	int *history_valid;
	unsigned char *accumulation;

// a thread for the GUI
	TimeAvgThread *thread;
	TimeAvgConfig config;
	int history_size;
// Starting frame of history in requested framerate
	int64_t history_start;
// When subtraction is disabled, this detects no change for paranoid mode.
	int64_t prev_frame;

	BC_Hash *defaults;
};


#endif

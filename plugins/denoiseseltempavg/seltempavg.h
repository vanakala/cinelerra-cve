
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

#ifndef SELTEMPAVG_H
#define SELTEMPAVG_H

class SelTempAvgMain;

#include "bchash.inc"
#include "pluginvclient.h"
#include "seltempavgwindow.h"
#include "vframe.inc"

class SelTempAvgConfig
{
public:
	SelTempAvgConfig();
	void copy_from(SelTempAvgConfig *src);
	int equivalent(SelTempAvgConfig *src);

	int frames;

	float avg_threshold_RY, avg_threshold_GU, avg_threshold_BV;
	float std_threshold_RY, std_threshold_GU, std_threshold_BV;
	int mask_RY, mask_GU, mask_BV;

	int method;
	enum
	{
		METHOD_NONE,
		METHOD_SELTEMPAVG,
		METHOD_AVERAGE,
		METHOD_STDDEV
	};

	int offsetmode;
	enum
	{
		OFFSETMODE_FIXED,
		OFFSETMODE_RESTARTMARKERSYS
	};


	int paranoid;
	int nosubtract;
	int offset_restartmarker_keyframe;
	int offset_fixed_value;
	float gain;

};


class SelTempAvgMain : public PluginVClient
{
public:
	SelTempAvgMain(PluginServer *server);
	~SelTempAvgMain();

	PLUGIN_CLASS_MEMBERS(SelTempAvgConfig, SelTempAvgThread);

// required for all realtime plugins
	int process_buffer(VFrame *frame, framenum start_position, double frame_rate);
	int is_realtime();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int nextkeyframeisoffsetrestart(KeyFrame *keyframe);
	void update_gui();
	void clear_accum(int w, int h, int color_model);
	void subtract_accum(VFrame *frame);
	void add_accum(VFrame *frame);
	void transfer_accum(VFrame *frame);

	framenum restartoffset;
	int onakeyframe;

	char string[64];

	VFrame **history;

// Frame of history in requested framerate
	framenum *history_frame;
	int *history_valid;
	unsigned char *accumulation;
	unsigned char *accumulation_sq;
	unsigned char *accumulation_grey;

	int history_size;
// Starting frame of history in requested framerate
	framenum history_start;
// When subtraction is disabled, this detects no change for paranoid mode.
	framenum prev_frame;
};


#endif

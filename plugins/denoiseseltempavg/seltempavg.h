
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

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

// Old name: "Selective Temporal Averaging"
#define PLUGIN_TITLE N_("STA")
#define PLUGIN_CLASS SelTempAvgMain
#define PLUGIN_CONFIG_CLASS SelTempAvgConfig
#define PLUGIN_THREAD_CLASS SelTempAvgThread
#define PLUGIN_GUI_CLASS SelTempAvgWindow

#include "pluginmacros.h"

#include "bchash.inc"
#include "pluginvclient.h"
#include "language.h"
#include "seltempavgwindow.h"
#include "vframe.inc"

class SelTempAvgConfig
{
public:
	SelTempAvgConfig();
	void copy_from(SelTempAvgConfig *src);
	int equivalent(SelTempAvgConfig *src);
// oli frames
	ptstime duration;

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
	ptstime offset_fixed_pts;
	float gain;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class SelTempAvgMain : public PluginVClient
{
public:
	SelTempAvgMain(PluginServer *server);
	~SelTempAvgMain();

	PLUGIN_CLASS_MEMBERS

// required for all realtime plugins
	void process_frame(VFrame *frame);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int nextkeyframeisoffsetrestart(KeyFrame *keyframe);
	void clear_accum(int w, int h, int color_model);
	void subtract_accum(VFrame *frame);
	void add_accum(VFrame *frame);
	void transfer_accum(VFrame *frame);

	ptstime restartoffset;
	int onakeyframe;

	VFrame **history;
	VFrame *temp_frame;

// Frame of history in requested framerate
	int max_num_frames;
	int *history_valid;
	unsigned char *accumulation;
	unsigned char *accumulation_sq;
	unsigned char *accumulation_grey;

	int frames_accum;
// When subtraction is disabled, this detects no change for paranoid mode.
	ptstime prev_frame_pts;
	int max_denominator;
};

#endif

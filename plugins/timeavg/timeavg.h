// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef TIMEAVG_H
#define TIMEAVG_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

#define PLUGIN_TITLE N_("Time Average")
#define PLUGIN_CLASS TimeAvgMain
#define PLUGIN_CONFIG_CLASS TimeAvgConfig
#define PLUGIN_THREAD_CLASS TimeAvgThread
#define PLUGIN_GUI_CLASS TimeAvgWindow

#include "pluginmacros.h"

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

	ptstime duration;
	int mode;
	enum
	{
		AVERAGE,
		ACCUMULATE,
		OR
	};
	int paranoid;
	int nosubtract;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class TimeAvgMain : public PluginVClient
{
public:
	TimeAvgMain(PluginServer *server);
	~TimeAvgMain();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *frame);
	void reset_plugin();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void clear_accum(int w, int h, int color_model);
	void subtract_accum(VFrame *frame);
	void add_accum(VFrame *frame);
	void transfer_accum(VFrame *frame);

	VFrame **history;
	int max_num_frames;
	int *history_valid;
	int *accumulation;

	int frames_accum;
// When subtraction is disabled, this detects no change for paranoid mode.
	ptstime prev_frame_pts;
	int max_denominator;
};

#endif

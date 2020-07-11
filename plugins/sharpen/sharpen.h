// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef SHARPEN_H
#define SHARPEN_H

// Sharpen leaves the last line too bright
#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Sharpen")
#define PLUGIN_CLASS SharpenMain
#define PLUGIN_CONFIG_CLASS SharpenConfig
#define PLUGIN_THREAD_CLASS SharpenThread
#define PLUGIN_GUI_CLASS SharpenWindow

#include "pluginmacros.h"

#define MAXSHARPNESS 100
#define MAX_ENGINES  2
#define LUTS_SIZE    0x10000

#include "condition.inc"
#include "bchash.h"
#include "language.h"
#include "pluginvclient.h"
#include "sharpenwindow.h"

#include <sys/types.h>

class SharpenEngine;

class SharpenConfig
{
public:
	SharpenConfig();

	void copy_from(SharpenConfig &that);
	int equivalent(SharpenConfig &that);
	void interpolate(SharpenConfig &prev, 
		SharpenConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int horizontal;
	int interlace;
	int luminance;
	double sharpness;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class SharpenMain : public PluginVClient
{
public:
	SharpenMain(PluginServer *server);
	~SharpenMain();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *input);
	void reset_plugin();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();

// parameters needed for sharpness
	int row_step;

	int *pos_lut, *neg_lut;

private:
	void get_luts();
	SharpenEngine *engine[MAX_ENGINES];
	int total_engines;
};


class SharpenEngine : public Thread
{
public:
	SharpenEngine(SharpenMain *plugin);
	~SharpenEngine();

	void start_process_frame(VFrame *output, int field);
	void wait_process_frame();
	void run();

	void filter(int w,
		u_int16_t *src,
		u_int16_t *dst,
		int *neg0,
		int *neg1,
		int *neg2);

	double calculate_pos(double value);
	double calculate_neg(double value);

	SharpenMain *plugin;
	int field;
	VFrame *output;
	int last_frame;
	Condition *input_lock, *output_lock;
	uint16_t *src_rows[4], *dst_row;
	int *neg_rows[4];
	double sharpness_coef;
};

#endif

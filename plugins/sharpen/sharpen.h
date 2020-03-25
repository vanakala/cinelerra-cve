
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
	float sharpness;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class SharpenMain : public PluginVClient
{
public:
	SharpenMain(PluginServer *server);
	~SharpenMain();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *input);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();

// parameters needed for sharpness
	int row_step;

	int pos_lut[0x10000], neg_lut[0x10000];

private:
	void get_luts(int *pos_lut, int *neg_lut, int color_model);
	SharpenEngine **engine;
	int total_engines;
};


class SharpenEngine : public Thread
{
public:
	SharpenEngine(SharpenMain *plugin, VFrame *input);
	~SharpenEngine();

	void start_process_frame(VFrame *output, int field);
	void wait_process_frame();
	void run();

	void filter(int components,
		int vmax,
		int w, 
		unsigned char *src, 
		unsigned char *dst,
		int *neg0, 
		int *neg1, 
		int *neg2);
	void filter(int components,
		int vmax,
		int w, 
		u_int16_t *src, 
		u_int16_t *dst,
		int *neg0, 
		int *neg1, 
		int *neg2);
	void filter(int components,
		int vmax,
		int w, 
		float *src, 
		float *dst,
		float *neg0, 
		float *neg1, 
		float *neg2);

	double calculate_pos(double value);
	double calculate_neg(double value);

	SharpenMain *plugin;
	int field;
	VFrame *output;
	int last_frame;
	Condition *input_lock, *output_lock;
	unsigned char *src_rows[4], *dst_row;
	unsigned char *neg_rows[4];
	double sharpness_coef;
};

#endif

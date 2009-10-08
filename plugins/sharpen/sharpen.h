
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

// the simplest plugin possible
// Sharpen leaves the last line too bright

class SharpenMain;
#define MAXSHARPNESS 100

#include "condition.inc"
#include "bchash.h"
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
		long prev_frame, 
		long next_frame, 
		long current_frame);

	
	
	int horizontal;
	int interlace;
	int luminance;
	float sharpness;
};

class SharpenMain : public PluginVClient
{
public:
	SharpenMain(PluginServer *server);
	~SharpenMain();

// required for all realtime plugins
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	char* plugin_title();
	int show_gui();
	void raise_window();
	int set_string();
	void update_gui();
	int load_configuration();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_defaults();
	int save_defaults();
	VFrame* new_picon();

// parameters needed for sharpness
	int row_step;

// a thread for the GUI
	SharpenThread *thread;
	int pos_lut[0x10000], neg_lut[0x10000];
	SharpenConfig config;
	VFrame *output, *input;

private:
	int get_luts(int *pos_lut, int *neg_lut, int color_model);
	BC_Hash *defaults;
	SharpenEngine **engine;
	int total_engines;
};


class SharpenEngine : public Thread
{
public:
	SharpenEngine(SharpenMain *plugin);
	~SharpenEngine();

	int start_process_frame(VFrame *output, VFrame *input, int field);
	int wait_process_frame();
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


	void filter_888(int w, 
		unsigned char *src, 
		unsigned char *dst,
		int *neg0, 
		int *neg1, 
		int *neg2);
	void filter_8888(int w, 
		unsigned char *src, 
		unsigned char *dst,
		int *neg0, 
		int *neg1, 
		int *neg2);
	void filter_161616(int w, 
		u_int16_t *src, 
		u_int16_t *dst,
		int *neg0, 
		int *neg1, 
		int *neg2);
	void filter_16161616(int w, 
		u_int16_t *src, 
		u_int16_t *dst,
		int *neg0, 
		int *neg1, 
		int *neg2);

	int filter(int w, 
		unsigned char *src, 
		unsigned char *dst, 
		int *neg0, 
		int *neg1, 
		int *neg2);

	float calculate_pos(float value);
	float calculate_neg(float value);


	SharpenMain *plugin;
	int field;
	VFrame *output, *input;
	int last_frame;
	Condition *input_lock, *output_lock;
	unsigned char *src_rows[4], *dst_row;
	unsigned char *neg_rows[4];
	float sharpness_coef;
};

#endif


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

#ifndef BLUR_H
#define BLUR_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME

#define PLUGIN_TITLE N_("Blur")
#define PLUGIN_CLASS BlurMain
#define PLUGIN_CONFIG_CLASS BlurConfig
#define PLUGIN_THREAD_CLASS BlurThread
#define PLUGIN_GUI_CLASS BlurWindow

#include "pluginmacros.h"

class BlurEngine;

#define MAXRADIUS 100

#include "language.h"
#include "mutex.h"
#include "pluginvclient.h"
#include "vframe.inc"

typedef struct
{
	float r;
	float g;
	float b;
	float a;
} pixel_f;

class BlurConfig
{
public:
	BlurConfig();

	int equivalent(BlurConfig &that);
	void copy_from(BlurConfig &that);
	void interpolate(BlurConfig &prev, 
		BlurConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int vertical;
	int horizontal;
	int radius;
	int a, r ,g ,b;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class BlurMain : public PluginVClient
{
public:
	BlurMain(PluginServer *server);
	~BlurMain();

// required for all realtime plugins
	void process_realtime(VFrame *input_ptr, VFrame *output_ptr);

	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	PLUGIN_CLASS_MEMBERS

	int need_reconfigure;

// a thread for the GUI
	VFrame *temp, *input, *output;

private:
	BlurEngine **engine;
};


class BlurEngine : public Thread
{
public:
	BlurEngine(BlurMain *plugin, int start_y, int end_y);
	~BlurEngine();

	void run();
	void start_process_frame(VFrame *output, VFrame *input);
	void wait_process_frame();

// parameters needed for blur
	void get_constants();
	void reconfigure();
	void transfer_pixels(pixel_f *src1, pixel_f *src2, pixel_f *dest, int size);
	void blur_strip3(int &size);
	void blur_strip4(int &size);

	int color_model;
	float vmax;
	pixel_f *val_p, *val_m, *vp, *vm;
	pixel_f *sp_p, *sp_m;
	float n_p[5], n_m[5];
	float d_p[5], d_m[5];
	float bd_p[5], bd_m[5];
	float std_dev;
	pixel_f *src, *dst;
	pixel_f initial_p;
	pixel_f initial_m;
	int terms;
	BlurMain *plugin;
// A margin is introduced between the input and output to give a seemless transition between blurs
	int start_in, start_out;
	int end_in, end_out;
	VFrame *output, *input;
	int last_frame;
	Mutex input_lock, output_lock;
};

#endif

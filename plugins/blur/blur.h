// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BLUR_H
#define BLUR_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

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
	double chnl0;
	double chnl1;
	double chnl2;
	double chnl3;
} pixel;


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
	int chan0;
	int chan1;
	int chan2;
	int chan3;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class BlurMain : public PluginVClient
{
public:
	BlurMain(PluginServer *server);
	~BlurMain();

	VFrame *process_tmpframe(VFrame *input_ptr);
	void reset_plugin();

	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	PLUGIN_CLASS_MEMBERS

	VFrame *input;
	VFrame *temp;
private:
	int num_engines;
	BlurEngine **engine;
};


class BlurEngine : public Thread
{
public:
	BlurEngine(BlurMain *plugin, int start_y, int end_y);
	~BlurEngine();

	void run();
	void start_process_frame(VFrame *input);
	void wait_process_frame();

// parameters needed for blur
	void get_constants();
	void reconfigure();
	void transfer_pixels(pixel *src1, pixel *src2, pixel *dest, int size);
	void blur_strip4(int size);

	pixel *val_p, *val_m, *vp, *vm;
	pixel *sp_p, *sp_m;
	double n_p[5], n_m[5];
	double d_p[5], d_m[5];
	double bd_p[5], bd_m[5];
	double std_dev;
	pixel *src, *dst;
	pixel initial_p;
	pixel initial_m;
	BlurMain *plugin;
// A margin is introduced between the input and output to give a seemless transition between blurs
	int start_in, start_out;
	int end_in, end_out;
	VFrame *input;
	int last_frame;
	Mutex input_lock, output_lock;
};

#endif

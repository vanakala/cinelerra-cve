
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

#ifndef TIMESTRETCH_H
#define TIMESTRETCH_H

#include "bchash.inc"
#include "fourier.h"
#include "guicast.h"
#include "mainprogress.inc"
#include "pluginaclient.h"
#include "resample.inc"
#include "vframe.inc"

class TimeStretch;
class TimeStretchWindow;
class TimeStretchConfig;


class TimeStretchScale : public BC_FPot
{
public:
	TimeStretchScale(TimeStretch *plugin, int x, int y);
	int handle_event();
	TimeStretch *plugin;
};

class TimeStretchWindow : public BC_Window
{
public:
	TimeStretchWindow(TimeStretch *plugin, int x, int y);
	void create_objects();
	void update();
	TimeStretchScale *scale;
	TimeStretch *plugin;
};

PLUGIN_THREAD_HEADER(TimeStretch, TimeStretchThread, TimeStretchWindow)


class TimeStretchConfig
{
public:
	TimeStretchConfig();

	int equivalent(TimeStretchConfig &that);
	void copy_from(TimeStretchConfig &that);
	void interpolate(TimeStretchConfig &prev, 
		TimeStretchConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	double scale;
};


class PitchEngine : public CrossfadeFFT
{
public:
	PitchEngine(TimeStretch *plugin, int window_size);
	~PitchEngine();
	void get_frame(AFrame *frame);
	void signal_process_oversample(int reset);

	TimeStretch *plugin;
	AFrame *temp;
	double *input_buffer;
	int input_size;
	int input_allocated;
	samplenum current_input_sample;
	ptstime current_output_pts;

	double *last_phase;
	double *new_freq;
	double *new_magn;
	double *sum_phase;
	double *anal_freq;
	double *anal_magn;
};

class TimeStretch : public PluginAClient
{
public:
	TimeStretch(PluginServer *server);
	~TimeStretch();

	PLUGIN_CLASS_MEMBERS(TimeStretchConfig, TimeStretchThread);

	int is_realtime();
	int has_pts_api();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	void process_frame(AFrame *aframe);

	void load_defaults();
	void save_defaults();

	void update_gui();

	PitchEngine *pitch;
	Resample *resample;
	double *temp;
	int temp_allocated;
	double *input;
	int input_allocated;
};

#endif

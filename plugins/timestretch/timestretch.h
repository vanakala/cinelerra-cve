
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

#define PLUGIN_TITLE N_("Time stretch")
#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_CLASS TimeStretch
#define PLUGIN_CONFIG_CLASS TimeStretchConfig
#define PLUGIN_THREAD_CLASS TimeStretchThread
#define PLUGIN_GUI_CLASS TimeStretchWindow

#include "pluginmacros.h"
#include "bchash.inc"
#include "fourier.h"
#include "guicast.h"
#include "language.h"
#include "mainprogress.inc"
#include "pluginaclient.h"
#include "pluginwindow.h"
#include "resample.inc"
#include "vframe.inc"


class TimeStretchScale : public BC_FPot
{
public:
	TimeStretchScale(TimeStretch *plugin, int x, int y);
	int handle_event();
	TimeStretch *plugin;
};

class TimeStretchWindow : public PluginWindow
{
public:
	TimeStretchWindow(TimeStretch *plugin, int x, int y);

	void update();

	TimeStretchScale *scale;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER


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
	PLUGIN_CONFIG_CLASS_MEMBERS
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

	PLUGIN_CLASS_MEMBERS

	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	void process_frame(AFrame *aframe);

	void load_defaults();
	void save_defaults();

	PitchEngine *pitch;
	Resample *resample;
	double *temp;
	int temp_allocated;
	double *input;
	int input_allocated;
};

#endif

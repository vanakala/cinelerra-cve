
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

#ifndef PITCH_H
#define PITCH_H



#include "bchash.inc"
#include "fourier.h"
#include "guicast.h"
#include "mutex.h"
#include "pluginaclient.h"
#include "vframe.inc"


class PitchEffect;

class PitchScale : public BC_FPot
{
public:
	PitchScale(PitchEffect *plugin, int x, int y);
	int handle_event();
	PitchEffect *plugin;
};

class PitchWindow : public BC_Window
{
public:
	PitchWindow(PitchEffect *plugin, int x, int y);
	void create_objects();
	void update();
	void close_event();
	PitchScale *scale;
	PitchEffect *plugin;
};

PLUGIN_THREAD_HEADER(PitchEffect, PitchThread, PitchWindow)


class PitchConfig
{
public:
	PitchConfig();

	int equivalent(PitchConfig &that);
	void copy_from(PitchConfig &that);
	void interpolate(PitchConfig &prev, 
		PitchConfig &next, 
		posnum prev_frame, 
		posnum next_frame, 
		posnum current_frame);

	double scale;
};

class PitchFFT : public CrossfadeFFT
{
public:
	PitchFFT(PitchEffect *plugin);
	~PitchFFT();
	int signal_process_oversample(int reset);
	int read_samples(samplenum output_sample,
		int samples, 
		double *buffer);

	PitchEffect *plugin;

	double *last_phase;
	double *new_freq;
	double *new_magn;
	double *sum_phase;
	double *anal_freq;
	double *anal_magn;
};

class PitchEffect : public PluginAClient
{
public:
	PitchEffect(PluginServer *server);
	~PitchEffect();

	PLUGIN_CLASS_MEMBERS(PitchConfig, PitchThread);

	int is_realtime();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);

	int process_buffer(int size, 
		double *buffer,
		samplenum start_position,
		int sample_rate);

	void load_defaults();
	void save_defaults();
	void reset();
	void update_gui();

	PitchFFT *fft;
};

#endif


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

#define PLUGIN_TITLE N_("Pitch shift")
#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_CLASS PitchEffect
#define PLUGIN_CONFIG_CLASS PitchConfig
#define PLUGIN_THREAD_CLASS PitchThread
#define PLUGIN_GUI_CLASS PitchWindow

#include "pluginmacros.h"
#include "bchash.inc"
#include "fourier.h"
#include "guicast.h"
#include "mutex.h"
#include "pluginaclient.h"
#include "vframe.inc"


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

	void update();
	PitchScale *scale;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class PitchConfig
{
public:
	PitchConfig();

	int equivalent(PitchConfig &that);
	void copy_from(PitchConfig &that);
	void interpolate(PitchConfig &prev, 
		PitchConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	double scale;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class PitchFFT : public CrossfadeFFT
{
public:
	PitchFFT(PitchEffect *plugin, int window_size);
	~PitchFFT();
	void signal_process_oversample(int reset);
	void get_frame(AFrame *aframe);

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

	PLUGIN_CLASS_MEMBERS

	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);

	void process_frame(AFrame *aframe);

	void load_defaults();
	void save_defaults();

	PitchFFT *fft;
};

#endif

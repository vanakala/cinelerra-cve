
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

#ifndef REVERB_H
#define REVERB_H
// Old name was "Heroine College Concert Hall"
#define PLUGIN_TITLE N_("HC Concert Hall")

#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_IS_MULTICHANNEL
#define PLUGIN_IS_SYNTHESIS
#define PLUGIN_CLASS Reverb
#define PLUGIN_CONFIG_CLASS ReverbConfig
#define PLUGIN_THREAD_CLASS ReverbThread
#define PLUGIN_GUI_CLASS ReverbWindow

class ReverbEngine;

#include "pluginmacros.h"
#include "language.h"
#include "reverbwindow.h"
#include "pluginaclient.h"

class ReverbConfig
{
public:
	ReverbConfig();

	int equivalent(ReverbConfig &that);
	void copy_from(ReverbConfig &that);
	void interpolate(ReverbConfig &prev, 
		ReverbConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);
	void dump();

	double level_init;
	int delay_init;
	double ref_level1;
	double ref_level2;
	int ref_total;
	int ref_length;
	int lowpass1, lowpass2;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class Reverb : public PluginAClient
{
public:
	Reverb(PluginServer *server);
	~Reverb();

	PLUGIN_CLASS_MEMBERS

	AFrame **main_in;
	AFrame **main_out;
	double **dsp_in;
	int **ref_channels, **ref_offsets, **ref_lowpass;
	double **ref_levels;
	int dsp_in_length;
	int redo_buffers;
// skirts for lowpass filter
	double **lowpass_in1, **lowpass_in2;
	DB db;

// required for all realtime/multichannel plugins
	void process_realtime(AFrame **input_ptr, AFrame **output_ptr);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

// non realtime support
	void load_defaults();
	void save_defaults();

	ReverbEngine **engine;
	int initialized;
};

class ReverbEngine : public Thread
{
public:
	ReverbEngine(Reverb *plugin);
	~ReverbEngine();

	int process_overlay(double *in, double *out, 
		double &out1, double &out2, 
		double level, int lowpass, int samplerate, int size);
	int process_overlays(int output_buffer, int size);
	int wait_process_overlays();
	void run();

	Mutex input_lock, output_lock;
	int completed;
	int output_buffer;
	int size;
	Reverb *plugin;
};

#endif

// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef REVERB_H
#define REVERB_H
// Old name was "Heroine College Concert Hall"
#define PLUGIN_TITLE N_("HC Concert Hall")

#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_IS_MULTICHANNEL
#define PLUGIN_IS_SYNTHESIS
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_CLASS Reverb
#define PLUGIN_CONFIG_CLASS ReverbConfig
#define PLUGIN_THREAD_CLASS ReverbThread
#define PLUGIN_GUI_CLASS ReverbWindow

class ReverbEngine;

#include "pluginmacros.h"
#include "language.h"
#include "mutex.h"
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

	void reset_plugin();
	void process_tmpframes(AFrame **input_ptr);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	void load_defaults();
	void save_defaults();

	PLUGIN_CLASS_MEMBERS

	AFrame **main_in;
	double **dsp_in;
	int **ref_channels;
	int **ref_offsets;
	int **ref_lowpass;
	double **ref_levels;
	int dsp_in_length;
// skirts for lowpass filter
	double **lowpass_in1;
	double **lowpass_in2;

	ReverbEngine **engine;
	int smp_num;
	int allocated_buffers;
	int allocated_refs;
};

class ReverbEngine : public Thread
{
public:
	ReverbEngine(Reverb *plugin);
	~ReverbEngine();

	void process_overlay(double *in, double *out,
		double &out1, double &out2, 
		double level, int lowpass, int samplerate, int size);
	void process_overlays(int output_buffer, int size);
	void wait_process_overlays();
	void run();

	Mutex input_lock;
	Mutex output_lock;
	int completed;
	int output_buffer;
	int size;
	Reverb *plugin;
};

#endif

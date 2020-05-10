// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef TIMESTRETCH_H
#define TIMESTRETCH_H

#define PLUGIN_TITLE N_("Time stretch")
#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION
#define PLUGIN_CLASS TimeStretch
#define PLUGIN_CONFIG_CLASS TimeStretchConfig
#define PLUGIN_THREAD_CLASS TimeStretchThread
#define PLUGIN_GUI_CLASS TimeStretchWindow

#include "pluginmacros.h"

#include "aframe.inc"
#include "bcpot.h"
#include "fourier.h"
#include "language.h"
#include "pluginaclient.h"
#include "pluginwindow.h"
#include "resample.inc"


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

	double scale;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class PitchEngine : public Fourier
{
public:
	PitchEngine(TimeStretch *plugin, int window_size);
	~PitchEngine();

	int signal_process();

	int oversample;
	int reset_phase;
	TimeStretch *plugin;

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
	AFrame *process_tmpframe(AFrame *aframe);
	void calculate_pts();

	void load_defaults();
	void save_defaults();

	ptstime input_pts;
	ptstime prev_frame;
	ptstime prev_input;
	PitchEngine *pitch;
	Resample *resample;
	AFrame *input_frame;
};

#endif

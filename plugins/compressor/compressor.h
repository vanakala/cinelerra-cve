// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#define PLUGIN_TITLE N_("Compressor")
#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_IS_MULTICHANNEL
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_CLASS CompressorEffect
#define PLUGIN_CONFIG_CLASS CompressorConfig
#define PLUGIN_THREAD_CLASS CompressorThread
#define PLUGIN_GUI_CLASS CompressorWindow

#include "aframe.inc"
#include "bcbutton.h"
#include "bcpopupmenu.h"
#include "bcpot.h"
#include "bctextbox.h"
#include "bctoggle.h"
#include "pluginmacros.h"
#include "bchash.inc"
#include "cinelerra.h"
#include "language.h"
#include "pluginaclient.h"
#include "pluginwindow.h"


class CompressorCanvas : public BC_SubWindow
{
public:
	CompressorCanvas(CompressorEffect *plugin, int x, int y, int w, int h);

	int button_press_event();
	int button_release_event();
	int cursor_motion_event();

	enum
	{
		NONE,
		DRAG
	};

	int current_point;
	int current_operation;
	CompressorEffect *plugin;
};


class CompressorDecay : public BC_FPot
{
public:
	CompressorDecay(CompressorEffect *plugin, int x, int y);

	int handle_event();

	CompressorEffect *plugin;
};


class CompressorReaction : public BC_FPot
{
public:
	CompressorReaction(CompressorEffect *plugin, int x, int y);

	int handle_event();

	CompressorEffect *plugin;
	CompressorDecay *decay;
};

class CompressorClear : public BC_GenericButton
{
public:
	CompressorClear(CompressorEffect *plugin, int x, int y);

	int handle_event();

	CompressorEffect *plugin;
};

class CompressorX : public BC_TextBox
{
public:
	CompressorX(CompressorEffect *plugin, int x, int y);

	int handle_event();

	CompressorEffect *plugin;
};

class CompressorY : public BC_TextBox
{
public:
	CompressorY(CompressorEffect *plugin, int x, int y);

	int handle_event();

	CompressorEffect *plugin;
};

class CompressorTrigger : public BC_TextBox
{
public:
	CompressorTrigger(CompressorEffect *plugin, int x, int y);

	int handle_event();
	int button_press_event();

	CompressorEffect *plugin;
};

class CompressorSmooth : public BC_CheckBox
{
public:
	CompressorSmooth(CompressorEffect *plugin, int x, int y);

	int handle_event();

	CompressorEffect *plugin;
};

class CompressorInput : public BC_PopupMenu
{
public:
	CompressorInput(CompressorEffect *plugin, int x, int y);

	void create_objects();
	int handle_event();

	static const char* value_to_text(int value);
	static int text_to_value(const char *text);
	CompressorEffect *plugin;
};

class CompressorWindow : public PluginWindow
{
public:
	CompressorWindow(CompressorEffect *plugin, int x, int y);

	void update();
	void update_textboxes();
	void update_canvas();
	void draw_scales();

	CompressorCanvas *canvas;
	CompressorReaction *reaction;
	CompressorClear *clear;
	CompressorX *x_text;
	CompressorY *y_text;
	CompressorTrigger *trigger;
	CompressorDecay *decay;
	CompressorSmooth *smooth;
	CompressorInput *input;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER


typedef struct
{
// DB from min_db - 0
	double x, y;
} compressor_point_t;

class CompressorConfig
{
public:
	CompressorConfig();

	void copy_from(CompressorConfig &that);
	int equivalent(CompressorConfig &that);
	void interpolate(CompressorConfig &prev, 
		CompressorConfig &next, 
		ptstime prev_pts,
		ptstime next_pts, 
		ptstime current_pts);

	int total_points();
	void remove_point(int number);
	void optimize();
// Return values of a specific point
	double get_y(int number);
	double get_x(int number);
// Returns db output from db input
	double calculate_db(double x);
	int set_point(double x, double y);

	int trigger;
	int input;
	enum
	{
		TRIGGER,
		MAX,
		SUM
	};
	double min_db;
	double reaction_len;
	double decay_len;
	double min_x, min_y;
	double max_x, max_y;
	int smoothing_only;
	ArrayList<compressor_point_t> levels;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class CompressorEffect : public PluginAClient
{
public:
	CompressorEffect(PluginServer *server);
	~CompressorEffect();

	void reset_plugin();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	void process_tmpframes(AFrame **aframes);
	double calculate_gain(double input);

// Calculate linear output from linear input
	double calculate_output(double x);

	void load_defaults();
	void save_defaults();

	PLUGIN_CLASS_MEMBERS

// The raw input data for each channel with readahead
	double *input_buffer[MAXCHANNELS];
// Number of samples in the input buffer 
	int input_size;
// Number of samples allocated in the input buffer
	int input_allocated;
// Starting sample of input buffer relative to project in requested rate.
	samplenum input_start;

// ending input value of smoothed input
	double next_target;
// starting input value of smoothed input
	double previous_target;
// samples between previous and next target value for readahead
	int target_samples;
// current sample from 0 to target_samples
	int target_current_sample;
// current smoothed input value
	double current_value;
// Temporaries for linear transfer
	ArrayList<compressor_point_t> levels;
	double min_x, min_y;
	double max_x, max_y;
};

#endif

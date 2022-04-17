// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef VIDEOSCOPE_H
#define VIDEOSCOPE_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_STATUS_GUI
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION
#define PLUGIN_NOT_KEYFRAMEABLE

#define PLUGIN_TITLE N_("VideoScope")
#define PLUGIN_CLASS  VideoScopeEffect
#define PLUGIN_CONFIG_CLASS VideoScopeConfig
#define PLUGIN_THREAD_CLASS VideoScopeThread
#define PLUGIN_GUI_CLASS VideoScopeWindow

#include "pluginmacros.h"

#include "bcbitmap.inc"
#include "language.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

#define WAVEFORM_DIVISIONS 12
#define VECTORSCOPE_DIVISIONS 12
#define RGB_MIN  48

// Vectorscope HSV axes and labels
const struct Vectorscope_HSV_axes
{
	int    hue;   // angle, degrees
	char   label[4];
	int    color; // label color
}

Vectorscope_HSV_axes[] =
	{
		{   0, "R",  RED      },
		{  60, "Yl", YELLOW   },
		{ 120, "G",  GREEN    },
		{ 180, "Cy", LTCYAN   },
		{ 240, "B",  BLUE     },
		{ 300, "Mg", MDPURPLE },
	};

const int Vectorscope_HSV_axes_count = sizeof(Vectorscope_HSV_axes) /
	sizeof(struct Vectorscope_HSV_axes);

class VideoScopeEngine;

class VideoScopeConfig
{
public:
	VideoScopeConfig();

	int show_srgb_limits;   // sRGB
	int show_ycbcr_limits;
	int draw_lines_inverse;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class VideoScopeGraduation
{
// One VideoScopeGraduation represents one line (or circle) and associated
// label. We use arrays of VideoScopeGraduations.
public:
	VideoScopeGraduation();
	void set(const char * label, int y);

	char label[4];
	int y;
};

class VideoScopeWaveform : public BC_SubWindow
{
public:
	VideoScopeWaveform(VideoScopeEffect *plugin, 
		int x, 
		int y,
		int w,
		int h);
	VideoScopeEffect *plugin;

	void calculate_graduations();
	void draw_graduations();
	void redraw();

	// All standard divisions + the one more at the end
	static const int NUM_GRADS = WAVEFORM_DIVISIONS + 1;
	VideoScopeGraduation  grads[NUM_GRADS];

	// Special limit lines are not always drawn, so they are separate.
	// They don't get labels (too crowded).
	int  limit_ycbcr_white;  // ITU-R B.601 235 = 92.2%
	int  limit_ycbcr_black;  // ITU-R B.601  16 =  6.3%
};


class VideoScopeVectorscope : public BC_SubWindow
{
public:
	VideoScopeVectorscope(VideoScopeEffect *plugin, 
		int x, 
		int y,
		int w,
		int h);
	VideoScopeEffect *plugin;

	void calculate_graduations();
	void draw_graduations();

	// Draw only every other division.
	static const int NUM_GRADS = VECTORSCOPE_DIVISIONS / 2;
	VideoScopeGraduation  grads[NUM_GRADS];

private:
	int color_axis_font;
	struct {
		int x1, y1, x2, y2;
		int text_x, text_y;
	} axes[Vectorscope_HSV_axes_count];
};

class VideoScopeShowSRGBLimits : public BC_CheckBox
{
public:
	VideoScopeShowSRGBLimits(VideoScopeEffect *plugin,
		int x,
		int y);
	int handle_event();
	VideoScopeEffect *plugin;
};

class VideoScopeShowYCbCrLimits : public BC_CheckBox
{
public:
	VideoScopeShowYCbCrLimits(VideoScopeEffect *plugin,
		int x,
		int y);
	int handle_event();
	VideoScopeEffect *plugin;
};

class VideoScopeDrawLinesInverse : public BC_CheckBox
{
public:
	VideoScopeDrawLinesInverse(VideoScopeEffect *plugin,
		int x,
		int y);
	int handle_event();
	VideoScopeEffect *plugin;
};

class VideoScopeWindow : public PluginWindow
{
public:
	VideoScopeWindow(VideoScopeEffect *plugin, int x, int y);
	~VideoScopeWindow();

	void calculate_sizes(int w, int h);
	int get_label_width();
	int get_widget_area_height();
	void allocate_bitmaps();
	void draw_labels();
	void update() {};

	VideoScopeWaveform *waveform;
	VideoScopeVectorscope *vectorscope;
	VideoScopeShowSRGBLimits *show_srgb_limits;
	VideoScopeShowYCbCrLimits *show_ycbcr_limits;
	VideoScopeDrawLinesInverse *draw_lines_inverse;
	BC_Bitmap *waveform_bitmap;
	BC_Bitmap *vector_bitmap;

	int vector_x, vector_y, vector_w, vector_h;
	int wave_x, wave_y, wave_w, wave_h;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class VideoScopePackage : public LoadPackage
{
public:
	VideoScopePackage();
	int row1, row2;
};

class VideoScopeUnit : public LoadClient
{
public:
	VideoScopeUnit(VideoScopeEffect *plugin, VideoScopeEngine *server);

	void process_package(LoadPackage *package);
	VideoScopeEffect *plugin;
};

class VideoScopeEngine : public LoadServer
{
public:
	VideoScopeEngine(VideoScopeEffect *plugin, int cpus);

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	VideoScopeEffect *plugin;
};

class VideoScopeEffect : public PluginVClient
{
public:
	VideoScopeEffect(PluginServer *server);
	~VideoScopeEffect();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *input);
	void reset_plugin();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void render_gui(void *input);

	VFrame *input;
	VideoScopeEngine *engine;
};

#endif

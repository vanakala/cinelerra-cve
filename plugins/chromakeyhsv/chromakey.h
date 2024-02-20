// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef CHROMAKEY_H
#define CHROMAKEY_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Chroma key (HSV)")
#define PLUGIN_CLASS ChromaKeyHSV
#define PLUGIN_CONFIG_CLASS ChromaKeyConfig
#define PLUGIN_THREAD_CLASS ChromaKeyThread
#define PLUGIN_GUI_CLASS ChromaKeyWindow

#include "pluginmacros.h"

#include "bcbutton.h"
#include "bcslider.h"
#include "bctoggle.h"
#include "colorpicker.h"
#include "language.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "pluginwindow.h"

class ChromaKeyConfig
{
public:
	ChromaKeyConfig();

	void copy_from(ChromaKeyConfig &src);
	int equivalent(ChromaKeyConfig &src);
	void interpolate(ChromaKeyConfig &prev, 
		ChromaKeyConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);
	int get_color();

	// Output mode
	int show_mask;
	// Key color definition
	int red;
	int green;
	int blue;
	// Key shade definition
	double min_brightness;
	double max_brightness;
	double saturation;
	double min_saturation;
	double tolerance;
	// Mask feathering
	double in_slope;
	double out_slope;
	double alpha_offset;
	// Spill light compensation
	double spill_threshold;
	double spill_amount;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class ChromaKeyColor : public BC_GenericButton
{
public:
	ChromaKeyColor(ChromaKeyHSV *plugin, 
		ChromaKeyWindow *gui, 
		int x, 
		int y);

	int handle_event();

	ChromaKeyHSV *plugin;
	ChromaKeyWindow *gui;
};

class ChromaKeySlider : public BC_FSlider
{
public:
	ChromaKeySlider(int x, int y, double min, double max,
		double *value, ChromaKeyHSV *plugin);

	int handle_event();

	ChromaKeyHSV *plugin;
	double *value;
};


class ChromaKeyUseColorPicker : public BC_GenericButton
{
public:
	ChromaKeyUseColorPicker(ChromaKeyHSV *plugin, ChromaKeyWindow *gui, int x, int y);

	int handle_event();

	ChromaKeyHSV *plugin;
	ChromaKeyWindow *gui;
};


class ChromaKeyColorThread : public ColorThread
{
public:
	ChromaKeyColorThread(ChromaKeyHSV *plugin, ChromaKeyWindow *gui);

	int handle_new_color(int red, int green, int blue, int alpha);

	ChromaKeyHSV *plugin;
	ChromaKeyWindow *gui;
};

class ChromaKeyShowMask : public BC_CheckBox
{
public:
	ChromaKeyShowMask(ChromaKeyHSV *plugin, int x, int y);
	int handle_event();
	ChromaKeyHSV *plugin;
};


class ChromaKeyWindow : public PluginWindow
{
public:
	ChromaKeyWindow(ChromaKeyHSV *plugin, int x, int y);
	~ChromaKeyWindow();

	void update();
	void update_sample();

	ChromaKeyColor *color;
	ChromaKeyUseColorPicker *use_colorpicker;
	ChromaKeySlider *min_brightness;
	ChromaKeySlider *max_brightness;
	ChromaKeySlider *saturation;
	ChromaKeySlider *min_saturation;
	ChromaKeySlider *tolerance;
	ChromaKeySlider *in_slope;
	ChromaKeySlider *out_slope;
	ChromaKeySlider *alpha_offset;
	ChromaKeySlider *spill_threshold;
	ChromaKeySlider *spill_amount;
	ChromaKeyShowMask *show_mask;
	BC_SubWindow *sample;
	ChromaKeyColorThread *color_thread;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class ChromaKeyServer : public LoadServer
{
public:
	ChromaKeyServer(ChromaKeyHSV *plugin);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	ChromaKeyHSV *plugin;
};

class ChromaKeyPackage : public LoadPackage
{
public:
	ChromaKeyPackage();
	int y1, y2;
};

class ChromaKeyUnit : public LoadClient
{
public:
	ChromaKeyUnit(ChromaKeyHSV *plugin, ChromaKeyServer *server);

	void process_package(LoadPackage *package);

	ChromaKeyHSV *plugin;

};


class ChromaKeyHSV : public PluginVClient
{
public:
	ChromaKeyHSV(PluginServer *server);
	~ChromaKeyHSV();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *frame);

	void reset_plugin();
	void handle_opengl();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	VFrame *input;
	ChromaKeyServer *engine;
};

#endif

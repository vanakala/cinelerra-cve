// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef CHROMAKEY_H
#define CHROMAKEY_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Chroma key")
#define PLUGIN_CLASS ChromaKey
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

	int red;
	int green;
	int blue;
	double threshold;
	double slope;
	int use_value;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class ChromaKeyColor : public BC_GenericButton
{
public:
	ChromaKeyColor(ChromaKey *plugin, 
		ChromaKeyWindow *gui, 
		int x, 
		int y);

	int handle_event();

	ChromaKey *plugin;
	ChromaKeyWindow *gui;
};

class ChromaKeyThreshold : public BC_FSlider
{
public:
	ChromaKeyThreshold(ChromaKey *plugin, int x, int y);

	int handle_event();

	ChromaKey *plugin;
};

class ChromaKeySlope : public BC_FSlider
{
public:
	ChromaKeySlope(ChromaKey *plugin, int x, int y);

	int handle_event();

	ChromaKey *plugin;
};

class ChromaKeyUseValue : public BC_CheckBox
{
public:
	ChromaKeyUseValue(ChromaKey *plugin, int x, int y);

	int handle_event();

	ChromaKey *plugin;
};

class ChromaKeyUseColorPicker : public BC_GenericButton
{
public:
	ChromaKeyUseColorPicker(ChromaKey *plugin, ChromaKeyWindow *gui, int x, int y);

	int handle_event();

	ChromaKey *plugin;
	ChromaKeyWindow *gui;
};


class ChromaKeyColorThread : public ColorThread
{
public:
	ChromaKeyColorThread(ChromaKey *plugin, ChromaKeyWindow *gui);

	int handle_new_color(int red, int green, int blue, int alpha);

	ChromaKey *plugin;
	ChromaKeyWindow *gui;
};


class ChromaKeyWindow : public PluginWindow
{
public:
	ChromaKeyWindow(ChromaKey *plugin, int x, int y);
	~ChromaKeyWindow();

	void update();
	void update_sample();

	ChromaKeyColor *color;
	ChromaKeyThreshold *threshold;
	ChromaKeyUseValue *use_value;
	ChromaKeyUseColorPicker *use_colorpicker;
	ChromaKeySlope *slope;
	BC_SubWindow *sample;
	ChromaKeyColorThread *color_thread;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class ChromaKeyServer : public LoadServer
{
public:
	ChromaKeyServer(ChromaKey *plugin);

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	ChromaKey *plugin;
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
	ChromaKeyUnit(ChromaKey *plugin, ChromaKeyServer *server);

	void process_package(LoadPackage *package);

	ChromaKey *plugin;
};


class ChromaKey : public PluginVClient
{
public:
	ChromaKey(PluginServer *server);
	~ChromaKey();

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

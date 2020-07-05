// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef ZOOMBLUR_H
#define ZOOMBLUR_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Zoom Blur")
#define PLUGIN_CLASS ZoomBlurMain
#define PLUGIN_CONFIG_CLASS ZoomBlurConfig
#define PLUGIN_THREAD_CLASS ZoomBlurThread
#define PLUGIN_GUI_CLASS ZoomBlurWindow

#include "pluginmacros.h"

#include "bcslider.h"
#include "bctoggle.h"
#include "keyframe.inc"
#include "language.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

class ZoomBlurEngine;

class ZoomBlurConfig
{
public:
	ZoomBlurConfig();

	int equivalent(ZoomBlurConfig &that);
	void copy_from(ZoomBlurConfig &that);
	void interpolate(ZoomBlurConfig &prev, 
		ZoomBlurConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int x;
	int y;
	int radius;
	int steps;
	int chan0;
	int chan1;
	int chan2;
	int chan3;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class ZoomBlurSize : public BC_ISlider
{
public:
	ZoomBlurSize(ZoomBlurMain *plugin, 
		int x, 
		int y, 
		int *output,
		int min,
		int max);

	int handle_event();

	ZoomBlurMain *plugin;
	int *output;
};

class ZoomBlurToggle : public BC_CheckBox
{
public:
	ZoomBlurToggle(ZoomBlurMain *plugin, 
		int x, 
		int y, 
		int *output,
		char *string);

	int handle_event();

	ZoomBlurMain *plugin;
	int *output;
};

class ZoomBlurWindow : public PluginWindow
{
public:
	ZoomBlurWindow(ZoomBlurMain *plugin, int x, int y);

	void update();

	ZoomBlurSize *x, *y, *radius, *steps;
	ZoomBlurToggle *chan0;
	ZoomBlurToggle *chan1;
	ZoomBlurToggle *chan2;
	ZoomBlurToggle *chan3;
	PLUGIN_GUI_CLASS_MEMBERS
private:
	static const char *blur_chn_rgba[];
	static const char *blur_chn_ayuv[];
};


PLUGIN_THREAD_HEADER


// Output coords for a layer of blurring
// Used for OpenGL only
class ZoomBlurLayer
{
public:
	ZoomBlurLayer() {};
	float x1, y1, x2, y2;
};

class ZoomBlurMain : public PluginVClient
{
public:
	ZoomBlurMain(PluginServer *server);
	~ZoomBlurMain();

	VFrame *process_tmpframe(VFrame *frame);
	void reset_plugin();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void handle_opengl();

	PLUGIN_CLASS_MEMBERS

	void delete_tables();
	VFrame *input, *output;
	ZoomBlurEngine *engine;
	int **scale_y_table;
	int **scale_x_table;
	ZoomBlurLayer *layer_table;
	int table_entries;
// The accumulation buffer is needed because 8 bits isn't precise enough
	unsigned char *accum;
	int entries_in_use;
private:
	size_t accum_size;
};

class ZoomBlurPackage : public LoadPackage
{
public:
	ZoomBlurPackage();

	int y1, y2;
};

class ZoomBlurUnit : public LoadClient
{
public:
	ZoomBlurUnit(ZoomBlurEngine *server, ZoomBlurMain *plugin);

	void process_package(LoadPackage *package);

	ZoomBlurEngine *server;
	ZoomBlurMain *plugin;
};

class ZoomBlurEngine : public LoadServer
{
public:
	ZoomBlurEngine(ZoomBlurMain *plugin, 
		int total_clients, 
		int total_packages);

	void init_packages();

	LoadClient* new_client();
	LoadPackage* new_package();

	ZoomBlurMain *plugin;
};

#endif


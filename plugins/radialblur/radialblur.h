// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef RADIALBLUR_H
#define RADIALBLUR_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Radial Blur")
#define PLUGIN_CLASS RadialBlurMain
#define PLUGIN_CONFIG_CLASS RadialBlurConfig
#define PLUGIN_THREAD_CLASS RadialBlurThread
#define PLUGIN_GUI_CLASS RadialBlurWindow

#include "pluginmacros.h"

#include "affine.inc"
#include "bcslider.h"
#include "keyframe.inc"
#include "language.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"


class RadialBlurEngine;


class RadialBlurConfig
{
public:
	RadialBlurConfig();

	int equivalent(RadialBlurConfig &that);
	void copy_from(RadialBlurConfig &that);
	void interpolate(RadialBlurConfig &prev, 
		RadialBlurConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int x;
	int y;
	int steps;
	int angle;
	int chan0;
	int chan1;
	int chan2;
	int chan3;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class RadialBlurSize : public BC_ISlider
{
public:
	RadialBlurSize(RadialBlurMain *plugin, 
		int x, 
		int y, 
		int *output,
		int min,
		int max);
	int handle_event();
	RadialBlurMain *plugin;
	int *output;
};

class RadialBlurToggle : public BC_CheckBox
{
public:
	RadialBlurToggle(RadialBlurMain *plugin, 
		int x, 
		int y, 
		int *output,
		char *string);
	int handle_event();
	RadialBlurMain *plugin;
	int *output;
};

class RadialBlurWindow : public PluginWindow
{
public:
	RadialBlurWindow(RadialBlurMain *plugin, int x, int y);

	void update();

	RadialBlurSize *x, *y, *steps, *angle;
	RadialBlurToggle *chan0;
	RadialBlurToggle *chan1;
	RadialBlurToggle *chan2;
	RadialBlurToggle *chan3;
	PLUGIN_GUI_CLASS_MEMBERS
private:
	static const char *blur_chn_rgba[];
	static const char *blur_chn_ayuv[];
};


PLUGIN_THREAD_HEADER


class RadialBlurMain : public PluginVClient
{
public:
	RadialBlurMain(PluginServer *server);
	~RadialBlurMain();

	VFrame *process_tmpframe(VFrame *frame);
	void reset_plugin();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void handle_opengl();

	PLUGIN_CLASS_MEMBERS

	VFrame *input, *output;
	RadialBlurEngine *engine;
// Rotate engine only used for OpenGL
	AffineEngine *rotate;
};

class RadialBlurPackage : public LoadPackage
{
public:
	RadialBlurPackage();
	int y1, y2;
};

class RadialBlurUnit : public LoadClient
{
public:
	RadialBlurUnit(RadialBlurEngine *server, RadialBlurMain *plugin);
	void process_package(LoadPackage *package);
	RadialBlurEngine *server;
	RadialBlurMain *plugin;
};

class RadialBlurEngine : public LoadServer
{
public:
	RadialBlurEngine(RadialBlurMain *plugin, 
		int total_clients, 
		int total_packages);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	RadialBlurMain *plugin;
};

#endif
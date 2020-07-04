// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef LINEARBLUR_H
#define LINEARBLUR_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Linear Blur")
#define PLUGIN_CLASS LinearBlurMain
#define PLUGIN_CONFIG_CLASS LinearBlurConfig
#define PLUGIN_THREAD_CLASS LinearBlurThread
#define PLUGIN_GUI_CLASS LinearBlurWindow

#include "pluginmacros.h"

#include "bcslider.h"
#include "keyframe.inc"
#include "language.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

#ifdef HAVE_GL
#include <GL/gl.h>
#include <GL/glx.h>
#endif


class LinearBlurEngine;

class LinearBlurConfig
{
public:
	LinearBlurConfig();

	int equivalent(LinearBlurConfig &that);
	void copy_from(LinearBlurConfig &that);
	void interpolate(LinearBlurConfig &prev, 
		LinearBlurConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int radius;
	int steps;
	int angle;
	int chan0;
	int chan1;
	int chan2;
	int chan3;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class LinearBlurSize : public BC_ISlider
{
public:
	LinearBlurSize(LinearBlurMain *plugin, 
		int x, 
		int y, 
		int *output,
		int min,
		int max);
	int handle_event();
	LinearBlurMain *plugin;
	int *output;
};

class LinearBlurToggle : public BC_CheckBox
{
public:
	LinearBlurToggle(LinearBlurMain *plugin, 
		int x, 
		int y, 
		int *output,
		char *string);
	int handle_event();
	LinearBlurMain *plugin;
	int *output;
};

class LinearBlurWindow : public PluginWindow
{
public:
	LinearBlurWindow(LinearBlurMain *plugin, int x, int y);

	void update();

	LinearBlurSize *angle, *steps, *radius;
	LinearBlurToggle *chan0;
	LinearBlurToggle *chan1;
	LinearBlurToggle *chan2;
	LinearBlurToggle *chan3;
	PLUGIN_GUI_CLASS_MEMBERS
private:
	static const char *blur_chn_rgba[];
	static const char *blur_chn_ayuv[];
};


PLUGIN_THREAD_HEADER

/* FIXIT - OpenGl
// Output coords for a layer of blurring
// Used for OpenGL only
class LinearBlurLayer
{
public:
	LinearBlurLayer() {};
	int x, y;
};
	*/

class LinearBlurMain : public PluginVClient
{
public:
	LinearBlurMain(PluginServer *server);
	~LinearBlurMain();

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
	LinearBlurEngine *engine;
	int **scale_y_table;
	int **scale_x_table;
/* FIXIT - OpenGl
	LinearBlurLayer *layer_table;
	*/
	int table_entries;
	int steps_in_table;
// The accumulation buffer is needed because 8 bits isn't precise enough
	unsigned char *accum;
	size_t accum_size;
};

class LinearBlurPackage : public LoadPackage
{
public:
	LinearBlurPackage();

	int y1, y2;
};

class LinearBlurUnit : public LoadClient
{
public:
	LinearBlurUnit(LinearBlurEngine *server, LinearBlurMain *plugin);

	void process_package(LoadPackage *package);

	LinearBlurEngine *server;
	LinearBlurMain *plugin;
};

class LinearBlurEngine : public LoadServer
{
public:
	LinearBlurEngine(LinearBlurMain *plugin, 
		int total_clients, 
		int total_packages);

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	LinearBlurMain *plugin;
};

#endif

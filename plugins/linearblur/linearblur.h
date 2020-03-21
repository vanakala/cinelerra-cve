
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
#ifndef LINEARBLUR_H
#define LINEARBLUR_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME

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
	int r;
	int g;
	int b;
	int a;
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
	LinearBlurToggle *r, *g, *b, *a;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


// Output coords for a layer of blurring
// Used for OpenGL only
class LinearBlurLayer
{
public:
	LinearBlurLayer() {};
	int x, y;
};

class LinearBlurMain : public PluginVClient
{
public:
	LinearBlurMain(PluginServer *server);
	~LinearBlurMain();

	void process_frame(VFrame *frame);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void handle_opengl();

	PLUGIN_CLASS_MEMBERS

	void delete_tables();
	VFrame *input, *output, *temp;
	LinearBlurEngine *engine;
	int **scale_y_table;
	int **scale_x_table;
	LinearBlurLayer *layer_table;
	int table_entries;
	int need_reconfigure;
// The accumulation buffer is needed because 8 bits isn't precise enough
	unsigned char *accum;
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

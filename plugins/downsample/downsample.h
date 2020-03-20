
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

#ifndef DOWNSAMPLE_H
#define DOWNSAMPLE_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Downsample")
#define PLUGIN_CLASS DownSampleMain
#define PLUGIN_CONFIG_CLASS DownSampleConfig
#define PLUGIN_THREAD_CLASS DownSampleThread
#define PLUGIN_GUI_CLASS DownSampleWindow

#include "pluginmacros.h"

#include "bcslider.h"
#include "bctoggle.h"
#include "language.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

class DownSampleServer;

class DownSampleConfig
{
public:
	DownSampleConfig();

	int equivalent(DownSampleConfig &that);
	void copy_from(DownSampleConfig &that);
	void interpolate(DownSampleConfig &prev, 
		DownSampleConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int horizontal_x;
	int vertical_y;
	int horizontal;
	int vertical;
	int r;
	int g;
	int b;
	int a;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class DownSampleToggle : public BC_CheckBox
{
public:
	DownSampleToggle(DownSampleMain *plugin, 
		int x, 
		int y, 
		int *output, 
		char *string);
	int handle_event();
	DownSampleMain *plugin;
	int *output;
};

class DownSampleSize : public BC_ISlider
{
public:
	DownSampleSize(DownSampleMain *plugin, 
		int x, 
		int y, 
		int *output,
		int min,
		int max);
	int handle_event();
	DownSampleMain *plugin;
	int *output;
};

class DownSampleWindow : public PluginWindow
{
public:
	DownSampleWindow(DownSampleMain *plugin, int x, int y);

	void update();

	DownSampleToggle *r, *g, *b, *a;
	DownSampleSize *h, *v, *h_x, *v_y;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class DownSampleMain : public PluginVClient
{
public:
	DownSampleMain(PluginServer *server);
	~DownSampleMain();

	VFrame *process_tmpframe(VFrame *input_ptr);

	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	PLUGIN_CLASS_MEMBERS

	VFrame *output;
	DownSampleServer *engine;
};

class DownSamplePackage : public LoadPackage
{
public:
	DownSamplePackage();
	int y1, y2;
};

class DownSampleUnit : public LoadClient
{
public:
	DownSampleUnit(DownSampleServer *server, DownSampleMain *plugin);
	void process_package(LoadPackage *package);
	DownSampleServer *server;
	DownSampleMain *plugin;
};

class DownSampleServer : public LoadServer
{
public:
	DownSampleServer(DownSampleMain *plugin, 
		int total_clients, 
		int total_packages);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	DownSampleMain *plugin;
};

#endif

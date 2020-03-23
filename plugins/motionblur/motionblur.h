
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

#ifndef MOTIONBLUR_H
#define MOTIONBLUR_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Motion Blur")
#define PLUGIN_CLASS MotionBlurMain
#define PLUGIN_CONFIG_CLASS MotionBlurConfig
#define PLUGIN_THREAD_CLASS MotionBlurThread
#define PLUGIN_GUI_CLASS MotionBlurWindow

#include "pluginmacros.h"

#include "bchash.h"
#include "bcslider.h"
#include "keyframe.inc"
#include "language.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"


class MotionBlurEngine;


class MotionBlurConfig
{
public:
	MotionBlurConfig();

	int equivalent(MotionBlurConfig &that);
	void copy_from(MotionBlurConfig &that);
	void interpolate(MotionBlurConfig &prev, 
		MotionBlurConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int radius;
	int steps;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class MotionBlurSize : public BC_ISlider
{
public:
	MotionBlurSize(MotionBlurMain *plugin, 
		int x, 
		int y, 
		int *output,
		int min,
		int max);
	int handle_event();
	MotionBlurMain *plugin;
	int *output;
};


class MotionBlurWindow : public PluginWindow
{
public:
	MotionBlurWindow(MotionBlurMain *plugin, int x, int y);

	void update();

	MotionBlurSize *steps, *radius;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class MotionBlurMain : public PluginVClient
{
public:
	MotionBlurMain(PluginServer *server);
	~MotionBlurMain();

	VFrame *process_tmpframe(VFrame *input_ptr);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	PLUGIN_CLASS_MEMBERS

	void delete_tables();
	VFrame *input, *output;
	MotionBlurEngine *engine;
	int **scale_y_table;
	int **scale_x_table;
	int table_entries;
	unsigned char *accum;
	size_t accum_size;
	int table_steps;
};

class MotionBlurPackage : public LoadPackage
{
public:
	MotionBlurPackage();
	int y1, y2;
};

class MotionBlurUnit : public LoadClient
{
public:
	MotionBlurUnit(MotionBlurEngine *server, MotionBlurMain *plugin);
	void process_package(LoadPackage *package);
	MotionBlurEngine *server;
	MotionBlurMain *plugin;
};

class MotionBlurEngine : public LoadServer
{
public:
	MotionBlurEngine(MotionBlurMain *plugin, 
		int total_clients, 
		int total_packages);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	MotionBlurMain *plugin;
};

#endif

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

#ifndef LINEARIZE_H
#define LINEARIZE_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME

#define PLUGIN_TITLE N_("Gamma")
#define PLUGIN_CLASS GammaMain
#define PLUGIN_CONFIG_CLASS GammaConfig
#define PLUGIN_THREAD_CLASS GammaThread
#define PLUGIN_GUI_CLASS GammaWindow

#include "pluginmacros.h"

class GammaEngine;

#include "gammawindow.h"
#include "loadbalance.h"
#include "language.h"
#include "plugincolors.h"
#include "guicast.h"
#include "pluginvclient.h"

#define HISTOGRAM_SIZE 256

class GammaConfig
{
public:
	GammaConfig();

	int equivalent(GammaConfig &that);
	void copy_from(GammaConfig &that);
	void interpolate(GammaConfig &prev, 
		GammaConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	float max;
	float gamma;
	int automatic;
// Plot histogram
	int plot;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class GammaPackage : public LoadPackage
{
public:
	GammaPackage();
	int start, end;
};

class GammaUnit : public LoadClient
{
public:
	GammaUnit(GammaMain *plugin);
	void process_package(LoadPackage *package);
	GammaMain *plugin;
	int accum[HISTOGRAM_SIZE];
};

class GammaEngine : public LoadServer
{
public:
	GammaEngine(GammaMain *plugin);

	void process_packages(int operation, VFrame *data);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	VFrame *data;
	int operation;
	enum
	{
		HISTOGRAM,
		APPLY
	};
	GammaMain *plugin;
	int accum[HISTOGRAM_SIZE];
};

class GammaMain : public PluginVClient
{
public:
	GammaMain(PluginServer *server);
	~GammaMain();

// required for all realtime plugins
	void process_frame(VFrame *frame);

	void calculate_max(VFrame *frame);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();
	void render_gui(void *data);
	void handle_opengl();

	GammaEngine *engine;
	VFrame *frame;

	PLUGIN_CLASS_MEMBERS
};

#endif


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

#ifndef BRIGHTNESS_H
#define BRIGHTNESS_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Brightness/Contrast")
#define PLUGIN_CLASS BrightnessMain
#define PLUGIN_CONFIG_CLASS BrightnessConfig
#define PLUGIN_THREAD_CLASS BrightnessThread
#define PLUGIN_GUI_CLASS BrightnessWindow

#include "pluginmacros.h"

class BrightnessEngine;

#include "brightnesswindow.h"
#include "language.h"
#include "loadbalance.h"
#include "pluginvclient.h"

class BrightnessConfig
{
public:
	BrightnessConfig();

	int equivalent(BrightnessConfig &that);
	void copy_from(BrightnessConfig &that);
	void interpolate(BrightnessConfig &prev, 
		BrightnessConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	double brightness;
	double contrast;
	int luma;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class BrightnessMain : public PluginVClient
{
public:
	BrightnessMain(PluginServer *server);
	~BrightnessMain();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *frame);

	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();
	void handle_opengl();

	BrightnessEngine *engine;

	VFrame *input, *output;
};


class BrightnessPackage : public LoadPackage
{
public:
	BrightnessPackage();

	int row1, row2;
};

class BrightnessUnit : public LoadClient
{
public:
	BrightnessUnit(BrightnessEngine *server, BrightnessMain *plugin);

	void process_package(LoadPackage *package);

	BrightnessMain *plugin;
};

class BrightnessEngine : public LoadServer
{
public:
	BrightnessEngine(BrightnessMain *plugin, int cpus);

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	BrightnessMain *plugin;
};

#endif

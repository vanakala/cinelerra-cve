
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

class BrightnessEngine;
class BrightnessMain;

#include "brightnesswindow.h"
#include "loadbalance.h"
#include "plugincolors.h"
#include "pluginvclient.h"

class BrightnessConfig
{
public:
	BrightnessConfig();

	int equivalent(BrightnessConfig &that);
	void copy_from(BrightnessConfig &that);
	void interpolate(BrightnessConfig &prev, 
		BrightnessConfig &next, 
		posnum prev_frame, 
		posnum next_frame, 
		posnum current_frame);

	float brightness;
	float contrast;
	int luma;
};

class BrightnessMain : public PluginVClient
{
public:
	BrightnessMain(PluginServer *server);
	~BrightnessMain();

	PLUGIN_CLASS_MEMBERS(BrightnessConfig, BrightnessThread);

	int process_buffer(VFrame *frame,
		framenum start_position,
		double frame_rate);
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();
	void handle_opengl();

	BrightnessEngine *engine;
	int redo_buffers;
	static YUV yuv;

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
	~BrightnessUnit();

	void process_package(LoadPackage *package);

	BrightnessMain *plugin;
};

class BrightnessEngine : public LoadServer
{
public:
	BrightnessEngine(BrightnessMain *plugin, int cpus);
	~BrightnessEngine();

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	BrightnessMain *plugin;
};

#endif


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

#ifndef UNSHARP_H
#define UNSHARP_H

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bchash.inc"
#include "filexml.inc"
#include "keyframe.inc"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "unsharp.inc"
#include "unsharpwindow.inc"
#include "vframe.inc"



class UnsharpConfig
{
public:
	UnsharpConfig();

	int equivalent(UnsharpConfig &that);
	void copy_from(UnsharpConfig &that);
	void interpolate(UnsharpConfig &prev, 
		UnsharpConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	float radius;
	float amount;
	int threshold;
};




class UnsharpMain : public PluginVClient
{
public:
	UnsharpMain(PluginServer *server);
	~UnsharpMain();

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();

	PLUGIN_CLASS_MEMBERS(UnsharpConfig, UnsharpThread)

	UnsharpEngine *engine;
};


class UnsharpPackage : public LoadPackage
{
public:
	UnsharpPackage();
	int y1, y2;
};

class UnsharpUnit : public LoadClient
{
public:
	UnsharpUnit(UnsharpEngine *server, UnsharpMain *plugin);
	~UnsharpUnit();

	void process_package(LoadPackage *package);

	UnsharpEngine *server;
	UnsharpMain *plugin;
	VFrame *temp;
};

class UnsharpEngine : public LoadServer
{
public:
	UnsharpEngine(UnsharpMain *plugin, 
		int total_clients, 
		int total_packages);
	~UnsharpEngine();
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	void do_unsharp(VFrame *src);
	VFrame *src;

	UnsharpMain *plugin;
};




#endif







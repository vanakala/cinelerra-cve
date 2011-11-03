
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

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME

#define PLUGIN_TITLE N_("Unsharp")
#define PLUGIN_CLASS UnsharpMain
#define PLUGIN_CONFIG_CLASS UnsharpConfig
#define PLUGIN_THREAD_CLASS UnsharpThread
#define PLUGIN_GUI_CLASS UnsharpWindow

#include "pluginmacros.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bchash.inc"
#include "filexml.inc"
#include "keyframe.inc"
#include "language.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "unsharpwindow.inc"
#include "vframe.inc"

class UnsharpEngine;

class UnsharpConfig
{
public:
	UnsharpConfig();

	int equivalent(UnsharpConfig &that);
	void copy_from(UnsharpConfig &that);
	void interpolate(UnsharpConfig &prev, 
		UnsharpConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);
	float radius;
	float amount;
	int threshold;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class UnsharpMain : public PluginVClient
{
public:
	UnsharpMain(PluginServer *server);
	~UnsharpMain();

	void process_frame(VFrame *frame);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	PLUGIN_CLASS_MEMBERS

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

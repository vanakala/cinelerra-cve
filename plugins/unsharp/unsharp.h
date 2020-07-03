// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef UNSHARP_H
#define UNSHARP_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Unsharp")
#define PLUGIN_CLASS UnsharpMain
#define PLUGIN_CONFIG_CLASS UnsharpConfig
#define PLUGIN_THREAD_CLASS UnsharpThread
#define PLUGIN_GUI_CLASS UnsharpWindow

#include "pluginmacros.h"

#include "bchash.inc"
#include "filexml.inc"
#include "keyframe.inc"
#include "language.h"
#include "loadbalance.h"
#include "pluginvclient.h"
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
	double radius;
	double amount;
	int threshold;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class UnsharpMain : public PluginVClient
{
public:
	UnsharpMain(PluginServer *server);
	~UnsharpMain();

	VFrame *process_tmpframe(VFrame *frame);
	void reset_plugin();
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

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	void do_unsharp(VFrame *src);
	VFrame *src;

	UnsharpMain *plugin;
};

#endif

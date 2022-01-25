// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2007 Hermann Vosseler

#ifndef REROUTE_H
#define REROUTE_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_IS_MULTICHANNEL
#define PLUGIN_IS_SYNTHESIS
#define PLUGIN_MAX_CHANNELS 2
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_TITLE N_("Reroute")
#define PLUGIN_CLASS Reroute
#define PLUGIN_CONFIG_CLASS RerouteConfig
#define PLUGIN_THREAD_CLASS RerouteThread
#define PLUGIN_GUI_CLASS RerouteWindow

#include "pluginmacros.h"

#include "keyframe.inc"
#include "pluginserver.inc"
#include "pluginwindow.h"
#include "pluginvclient.h"
#include "selection.h"
#include "vframe.inc"

class Reroute;

class RerouteConfig
{
public:
	RerouteConfig();

	int equivalent(RerouteConfig &that);
	void copy_from(RerouteConfig &that);

	int operation;
	enum
	{
		REPLACE_ALL,
		REPLACE_COMPONENTS,
		REPLACE_ALPHA
	};

	PLUGIN_CONFIG_CLASS_MEMBERS
};

class RerouteSelection : public Selection
{
public:
	RerouteSelection(int x, int y, Reroute *plugin,
		BC_WindowBase *basewindow, int *value);

	int handle_event();
	void update(int value);
	static const char *name(int value);

private:
	Reroute *plugin;
	static struct selection_int reroute_operation[];
};


class RerouteWindow : public PluginWindow
{
public:
	RerouteWindow(Reroute *plugin, int x, int y);

	void update();

	PLUGIN_GUI_CLASS_MEMBERS
private:
	RerouteSelection *operation;
};

PLUGIN_THREAD_HEADER

class Reroute : public PluginVClient
{
public:
	Reroute(PluginServer *server);
	~Reroute();

	PLUGIN_CLASS_MEMBERS

	void process_tmpframes(VFrame **frame);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
};

#endif

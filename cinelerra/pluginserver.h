// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PLUGINSERVER_H
#define PLUGINSERVER_H

// inherited by plugins

#include "config.h"
#include "datatype.h"
#include "plugin.inc"
#include "pluginclient.inc"
#include "pluginserver.inc"
#include "theme.inc"
#include "trackrender.inc"
#include "vframe.inc"

#ifdef HAVE_LADSPA
#include <ladspa.h>
#endif

class PluginServer
{
public:
	PluginServer(const char *path);
	~PluginServer();

	friend class PluginAClientLAD;
	friend class PluginAClientConfig;
	friend class PluginAClientWindow;

// open a plugin and wait for commands
// Get information for plugindb if master.
	PluginClient *open_plugin(Plugin *plugin,
		TrackRender *renderer, int master = 0, int lad_index = -1);
// close the plugin
	void close_plugin(PluginClient *client);
// Dynamic loading
	int load_plugin();
	void release_plugin();

	void dump(int indent = 0);
// queries
	void set_title(const char *string);

	int plugin_open;                 // Whether or not the plugin is open.

// Specifies what type of plugin.
	int realtime, multichannel;
// Plugin generates media
	int synthesis;
// What data types the plugin can handle.  One of these is set.
	int audio, video, theme;
// Can display a GUI
	int uses_gui;
// Plugin is a transition
	int transition;
// Plugin api version
	int apiversion;
// Plugin supports OpenGL
	int opengl_plugin;
// Maximum number channels supported by multichannel plugin
	int multichannel_max;
// Plugin gui shows status of the current frame
	int status_gui;
// Plugin ignores keyframes
	int no_keyframes;
// name of plugin in english.
// Compared against the title value in the plugin for resolving symbols.
	char *title;
	char *path;           // location of plugin on disk

// Icon for Asset Window
	VFrame *picon;

private:
	void reset_parameters();

// Handle from dlopen.  Plugins are opened once at startup and stored in the master
// plugindb.
	void *plugin_fd;
// If no path, this is going to be set to a function which 
// instantiates the plugin.
	PluginClient* (*new_plugin)(PluginServer*);
#ifdef HAVE_LADSPA
// LAD support
	int is_lad;
	int ladspa_index;
	LADSPA_Descriptor_Function lad_descriptor_function;
	const LADSPA_Descriptor *lad_descriptor;
#endif
};

#endif

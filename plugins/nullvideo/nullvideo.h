// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2012 Einar Rünkaru <einarrunkaru@gmail dot com>

/*
 * Null Video plugin
 * The main purpose of this plugin is to be a skeletion
 * of an usable video effect
 */

#ifndef NULLVIDEO_H
#define NULLVIDEO_H

/*
 * Define the type of the plugin
 * This plugin is not multichannel, non realtime
 */
#define PLUGIN_IS_VIDEO

/*
 * Define the name and api related class names of the plugin
 */
#define PLUGIN_TITLE N_("Null")
#define PLUGIN_CLASS NullVideo
#define PLUGIN_GUI_CLASS NullVideoWindow
#define PLUGIN_USES_TMPFRAME

#include "pluginmacros.h"

#include "bctoggle.h"
#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

class NullVideoSwitch : public BC_Radial
{
public:
	NullVideoSwitch(NullVideo *plugin,
		int x,
		int y);

	int handle_event();
	NullVideo *plugin;
};

/*
 * Gui of the plugin - may be omitted
 */
class NullVideoWindow : public PluginWindow
{
public:
	NullVideoWindow(NullVideo *plugin, int x, int y);

	PLUGIN_GUI_CLASS_MEMBERS
};

/*
 * Main class of the plugin
 */
class NullVideo : public PluginVClient
{
public:
	NullVideo(PluginServer *server);
	~NullVideo();
/*
 * Macro defines members of the plugin class
 * Macro must be always used
 * Macro declares int load_configuration, if the plugin has confiuguration class
 */
	PLUGIN_CLASS_MEMBERS

// Processing is here
	int process_loop(VFrame **frames);

// Loading and saving of defaults - optional: needed if plugin has parameters
	void load_defaults();
	void save_defaults();

	int onoff;
	ptstime current_pts;
};

#endif

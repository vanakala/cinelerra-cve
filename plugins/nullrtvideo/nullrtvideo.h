// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2012 Einar Rünkaru <einarrunkaru@gmail dot com>

/*
 * Null Video realtime plugin
 * The main purpose of this plugin is to be a skeletion
 * of an usable video realtime effect
 */

#ifndef NULLRTVIDEO_H
#define NULLRTVIDEO_H

/*
 * Define the type of the plugin
 * This plugin is not multichannel
 */
#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

/*
 * We have own load_configiration (no need of parameter interpolation)
 */
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION
/*
 * Define the name and api related class names of the plugin
 */
#define PLUGIN_TITLE N_("NullRT")
#define PLUGIN_CLASS NRTVideo
#define PLUGIN_CONFIG_CLASS NRTVideoConfig
#define PLUGIN_THREAD_CLASS NRTVideoThread
#define PLUGIN_GUI_CLASS NRTVideoWindow

#include "pluginmacros.h"

#include "bctoggle.h"
#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

class NRTVideoConfig
{
public:
	NRTVideoConfig();
/*
 * If load_configuration from macros is used, next methods must be defined
 * int equivalent(NRTVideoConfig &that);
 * void copy_from(NRTVideoConfig &that);
 * void interpolate(NRTVideoConfig &prev, NRTVideoConfig &next
 *     ptstime prev_pts, ptstime next_pts, ptstime current_pts);
 */
	int onoff;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class NRTVideoSwitch : public BC_Radial
{
public:
	NRTVideoSwitch(NRTVideo *plugin,
		int x,
		int y);

	int handle_event();
	NRTVideo *plugin;
};

/*
 * Gui of the plugin - may be omitted
 */
class NRTVideoWindow : public PluginWindow
{
public:
	NRTVideoWindow(NRTVideo *plugin, int x, int y);

	void update();
	NRTVideoSwitch *toggle;
	PLUGIN_GUI_CLASS_MEMBERS
};

/*
 * Macro creates header part of the plugin thread class
 * Must be used if the plugin has a gui
 */
PLUGIN_THREAD_HEADER

/*
 * Main class of the plugin
 */
class NRTVideo : public PluginVClient
{
public:
	NRTVideo(PluginServer *server);
	~NRTVideo();
/*
 * Macro defines members of the plugin class
 * Macro must be always used
 * Macro declares int load_configuration, if the plugin has confiuguration class
 */
	PLUGIN_CLASS_MEMBERS
// Processing is here
	VFrame *process_tmpframe(VFrame *frame);
// Optional entry of an idle plugin
	void reset_plugin();
// Loading and saving of defaults - optional: needed if plugin has parameters
	void load_defaults();
	void save_defaults();
// Loading and saving parameters to the project - optional
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
};

#endif

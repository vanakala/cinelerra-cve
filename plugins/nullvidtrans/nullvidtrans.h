// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2012 Einar Rünkaru <einarrunkaru@gmail dot com>

/*
 * Null Video Transition
 * The main purpose of this plugin is to be a skeletion
 * of usable video transitions
 */

#ifndef NULLVIDTRANS_H
#define NULLVIDTRANS_H

/*
 * Define the type of the plugin
 */
#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_TRANSITION
#define PLUGIN_USES_TMPFRAME

/*
 * Define the name and api related class names of the plugin
 */
#define PLUGIN_TITLE N_("Null")
#define PLUGIN_CLASS NVTransition
#define PLUGIN_THREAD_CLASS NVTransitionThread
#define PLUGIN_GUI_CLASS NVTransitionWindow

#include "pluginmacros.h"

#include "bctoggle.h"
#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

class NVSwitch : public BC_Radial
{
public:
	NVSwitch(NVTransition *plugin,
		NVTransitionWindow *window,
		int x,
		int y);

	int handle_event();
	NVTransition *plugin;
	NVTransitionWindow *window;
};

/*
 * Gui of the plugin - may be omitted
 */
class NVTransitionWindow : public PluginWindow
{
public:
	NVTransitionWindow(NVTransition *plugin, int x, int y);

	NVSwitch *toggle;
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
class NVTransition : public PluginVClient
{
public:
	NVTransition(PluginServer *server);
	~NVTransition();
/*
 * Macro defines members of the plugin class
 * Macro must be always used
 */
	PLUGIN_CLASS_MEMBERS
// Processing is here
	void process_realtime(VFrame *incoming, VFrame *outgoing);
// Loading and saving of defaults - optional: needed if plugin has parameters
	void load_defaults();
	void save_defaults();
// Loading and saving parameters to the project - optional
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
// Calculates (interpolates) parameter values for current position
	int load_configuration();
/*
 * The parameter of the plugin
 */
	int onoff;
};

#endif

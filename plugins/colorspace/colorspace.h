// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2018 Einar Rünkaru <einarrunkaru at gmail dot com>

/*
 * Colorspace conversion test
 */

#ifndef COLORSPACE_H
#define COLORSPACE_H


// Define the type of the plugin
// This plugin is not multichannel

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

// We have own load_configiration (no need of parameter interpolation)
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

// Define the name and api related class names of the plugin
#define PLUGIN_TITLE N_("ColorSpace")
#define PLUGIN_CLASS ColorSpace
#define PLUGIN_CONFIG_CLASS ColorSpaceConfig
#define PLUGIN_THREAD_CLASS ColorSpaceThread
#define PLUGIN_GUI_CLASS ColorSpaceWindow

#include "pluginmacros.h"

#include "bctoggle.h"
#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

class ColorSpaceConfig
{
public:
	ColorSpaceConfig();

	int onoff;
	int avlibs;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class ColorSpaceSwitch : public BC_Radial
{
public:
	ColorSpaceSwitch(ColorSpace *plugin,
		int x,
		int y);

	int handle_event();
	ColorSpace *plugin;
};

class AVlibsSwitch : public BC_Radial
{
public:
	AVlibsSwitch(ColorSpace *plugin,
		int x,
		int y);

	int handle_event();
	ColorSpace *plugin;
};

class ColorSpaceWindow : public PluginWindow
{
public:
	ColorSpaceWindow(ColorSpace *plugin, int x, int y);

	void update();
	ColorSpaceSwitch *toggle;
	AVlibsSwitch *avltoggle;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class ColorSpace : public PluginVClient
{
public:
	ColorSpace(PluginServer *server);
	~ColorSpace();

	PLUGIN_CLASS_MEMBERS

// Processing is here
	VFrame *process_tmpframe(VFrame *frame);
// Loading and saving of defaults - optional: needed if plugin has parameters
	void load_defaults();
	void save_defaults();
// Loading and saving parameters to the project - optional
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
};

#endif

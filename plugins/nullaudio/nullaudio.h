// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2012 Einar Rünkaru <einarrunkaru@gmail dot com>

/*
 * Null Video plugin
 * The main purpose of this plugin is to be a skeletion
 * of an usable video effect
 */

#ifndef NULLAUDIO_H
#define NULLAUDIO_H

/*
 * Define the type of the plugin
 * This plugin is not multichannel, non realtime
 */
#define PLUGIN_IS_AUDIO

/*
 * Define the name and api related class names of the plugin
 */
#define PLUGIN_TITLE N_("Null")
#define PLUGIN_CLASS NullAudio
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_GUI_CLASS NullAudioWindow

#include "pluginmacros.h"

#include "bctoggle.h"
#include "language.h"
#include "pluginaclient.h"
#include "aframe.inc"
#include "pluginwindow.h"

class NullAudioSwitch : public BC_Radial
{
public:
	NullAudioSwitch(NullAudio *plugin,
		int x,
		int y);

	int handle_event();
	NullAudio *plugin;
};

/*
 * Gui of the plugin - may be omitted
 */
class NullAudioWindow : public PluginWindow
{
public:
	NullAudioWindow(NullAudio *plugin, int x, int y);

	PLUGIN_GUI_CLASS_MEMBERS
};

/*
 * Main class of the plugin
 */
class NullAudio : public PluginAClient
{
public:
	NullAudio(PluginServer *server);
	~NullAudio();
/*
 * Macro defines members of the plugin class
 * Macro must be always used
 * Macro declares int load_configuration, if the plugin has confiuguration class
 */
	PLUGIN_CLASS_MEMBERS

// Processing is here
	int process_loop(AFrame **frames);

// Loading and saving of defaults - optional: needed if plugin has parameters
	void load_defaults();
	void save_defaults();

	int onoff;
	MainProgressBar *progress;
	ptstime current_pts;
};

#endif

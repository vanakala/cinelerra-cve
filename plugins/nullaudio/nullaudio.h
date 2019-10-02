
/*
 * CINELERRA
 * Copyright (C) 2012 Einar Rünkaru <einarrunkaru@gmail dot com>
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

	void start_loop();
	void stop_loop();

// Processing is here
	int process_loop(AFrame *frame);

// Loading and saving of defaults - optional: needed if plugin has parameters
	void load_defaults();
	void save_defaults();

	int onoff;
	MainProgressBar *progress;
	ptstime current_pts;
};

#endif

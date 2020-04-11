
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
 * Null audio realtime plugin
 * The main purpose of this plugin is to be a skeletion
 * of an usable audio realtime effect
 */

#ifndef NULLRTAUDIO_H
#define NULLRTAUDIO_H

/*
 * Define the type of the plugin
 * This plugin is not multichannel
 */
#define PLUGIN_IS_AUDIO
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
#define PLUGIN_CLASS NRTAudio
#define PLUGIN_CONFIG_CLASS NRTAudioConfig
#define PLUGIN_THREAD_CLASS NRTAudioThread
#define PLUGIN_GUI_CLASS NRTAudioWindow

#include "pluginmacros.h"

#include "bctoggle.h"
#include "language.h"
#include "pluginaclient.h"
#include "vframe.inc"
#include "pluginwindow.h"

class NRTAudioConfig
{
public:
	NRTAudioConfig();
/*
 * If load_configuration from macros is used, next methods must be defined
 * int equivalent(NRTAudioConfig &that);
 * void copy_from(NRTAudioConfig &that);
 * void interpolate(NRTAudioConfig &prev, NRTAudioConfig &next
 *     ptstime prev_pts, ptstime next_pts, ptstime current_pts);
 */
	int onoff;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class NRTAudioSwitch : public BC_Radial
{
public:
	NRTAudioSwitch(NRTAudio *plugin,
		int x,
		int y);

	int handle_event();
	NRTAudio *plugin;
};

/*
 * Gui of the plugin - may be omitted
 */
class NRTAudioWindow : public PluginWindow
{
public:
	NRTAudioWindow(NRTAudio *plugin, int x, int y);

	void update();
	NRTAudioSwitch *toggle;
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
class NRTAudio : public PluginAClient
{
public:
	NRTAudio(PluginServer *server);
	~NRTAudio();
/*
 * Macro defines members of the plugin class
 * Macro must be always used
 * Macro declares int load_configuration, if the plugin has confiuguration class
 */
	PLUGIN_CLASS_MEMBERS
// Processing is here
	AFrame *process_tmpframe(AFrame *frame);
// Loading and saving of defaults - optional: needed if plugin has parameters
	void load_defaults();
	void save_defaults();
// Loading and saving parameters to the project - optional
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
};

#endif

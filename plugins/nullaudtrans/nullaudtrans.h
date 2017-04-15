
/*
 * CINELERRA
 * Copyright (C) 2012 Einar Rünkaru <einarry at smail dot ee>
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
 * Null Audio Transition
 * The main purpose of this plugin is to be a skeletion
 * of usable video transitions
 */

#ifndef NULLAUDTRANS_H
#define NULLAUDTRANS_H

/*
 * Define the type of the plugin
 */
#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_TRANSITION

/*
 * Define the name and api related class names of the plugin
 */
#define PLUGIN_TITLE N_("Null")
#define PLUGIN_CLASS NATransition
#define PLUGIN_THREAD_CLASS NATransitionThread
#define PLUGIN_GUI_CLASS NATransitionWindow

#include "pluginmacros.h"

#include "bctoggle.h"
#include "language.h"
#include "pluginaclient.h"
#include "vframe.inc"
#include "pluginwindow.h"

class NASwitch : public BC_Radial
{
public:
	NASwitch(NATransition *plugin,
		NATransitionWindow *window,
		int x,
		int y);

	int handle_event();
	NATransition *plugin;
	NATransitionWindow *window;
};

/*
 * Gui of the plugin - may be omitted
 */
class NATransitionWindow : public PluginWindow
{
public:
	NATransitionWindow(NATransition *plugin, int x, int y);

	NASwitch *toggle;
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
class NATransition : public PluginAClient
{
public:
	NATransition(PluginServer *server);
	~NATransition();
/*
 * Macro defines members of the plugin class
 * Macro must be always used
 */
	PLUGIN_CLASS_MEMBERS
// Processing is here
	void process_realtime(AFrame *incoming, AFrame *outgoing);
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


/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#ifndef PLUGINSERVER_H
#define PLUGINSERVER_H

// inherited by plugins

#include "aframe.inc"
#include "arraylist.h"
#include "datatype.h"
#include "edl.inc"
#include "guidelines.inc"
#include "keyframe.inc"
#include "mainprogress.inc"
#include "menueffects.inc"
#include "plugin.inc"
#include "pluginaclientlad.inc"
#include "pluginclient.inc"
#include "pluginserver.inc"
#include "renderengine.inc"
#include "theme.inc"
#include "track.inc"
#include "trackrender.inc"
#include "vframe.inc"
#include "videodevice.inc"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LADSPA
#include <ladspa.h>
#endif
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>


class PluginServer
{
public:
	PluginServer(const char *path);
	PluginServer(PluginServer &);
	virtual ~PluginServer();

	friend class PluginAClientLAD;
	friend class PluginAClientConfig;
	friend class PluginAClientWindow;

// open a plugin and wait for commands
// Get information for plugindb if master.
#define PLUGINSERVER_IS_LAD 2
#define PLUGINSERVER_NOT_RECOGNIZED 1
#define PLUGINSERVER_OK 0
	int open_plugin(int master, Plugin *plugin,
		TrackRender *renderer, int lad_index = -1);
// close the plugin
	void close_plugin();
// Dynamic loading
	int load_plugin();
	void release_plugin();

	void dump(int indent = 0);
// Release any objects which are required after playback stops.
	void render_stop();
// queries
	void set_title(const char *string);
// Generate title for display
	void generate_display_title(char *string);

// Get or create keyframe for writing, depending on whether auto keyframes
// is enabled.  Called by PluginClient::send_configure_change
	KeyFrame* get_keyframe();
// Create new theme object.  Used by theme plugins.
	Theme* new_theme();

// =============================== for realtime plugins
// save configuration of plugin
	void save_data(KeyFrame *keyframe);
// Update EDL and playback engines to reflect changes
	void sync_parameters();

// Raise the GUI
	void raise_window();
// cause the plugin to show the GUI
// Called by MWindow::show_plugin
	void show_gui();
	void hide_gui();
// Update GUI with keyframe settings
	void update_gui();
	void update_title();
// Set to 1 before every process call if the user supports OpenGL buffers.
// Also provides the driver location.
	void set_use_opengl(int value, VideoDevice *vdevice);
// Plugin must call this before performing OpenGL operations.
	int get_use_opengl();

// Called by plugin client to request synchronous routine.
	void run_opengl(PluginClient *plugin_client);

// give the buffers and prepare processing realtime data
	void init_realtime(int total_in_buffers);

// process the data in the buffers
// input - the current edit's data
// output - the previous edit's data and the destination of the transition output
// current_position - Position from start of the transition and 
//     relative to the transition.
// total_len - total len for transition
	void process_transition(VFrame *input, 
		VFrame *output, 
		ptstime current_postime,
		ptstime total_len);
	void process_transition(AFrame *input, 
		AFrame *output,
		ptstime current_postime,
		ptstime total_len);

// Process using pull method.
// frame/buffer - video/audio fame to process
// total_len - length of plugin
	void process_buffer(VFrame **frame, 
		ptstime total_len);
	void process_buffer(AFrame **buffer,
		ptstime total_len);

// Called by MWindow to cause GUI to display
	void render_gui(void *data);

// ============================ for non realtime plugins
// start processing data in plugin
	void start_loop(ptstime start, ptstime end, int buffer_size, int total_buffers);
// Do one iteration of a nonrealtime plugin and return if finished
	int process_loop(VFrame **buffers);
	int process_loop(AFrame **buffers);
	void stop_loop();

// For non realtime, prompt user for parameters, waits for plugin to finish and returns a result
	int get_parameters(ptstime start, ptstime end, int channels);
// Set pointer to a default keyframe when there is no plugin
	void set_keyframe(KeyFrame *keyframe);
// Set pointer to menueffect window
	void set_prompt(MenuEffectPrompt *prompt);
	void set_interactive();   // make this the master plugin for progress bars
	MainProgressBar* start_progress(char *string, ptstime length);

	int total_in_buffers;
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
// name of plugin in english.
// Compared against the title value in the plugin for resolving symbols.
	char *title;
	char *path;           // location of plugin on disk
	char *data_text;      // pointer to the data that was requested by a save_data command
	char *args[4];
	int total_args;

// Used by realtime read functions to get data.  Corresponds to the buffer table in the
// Pointer to keyframe when plugin is not available
	KeyFrame *keyframe;
// Storage of keyframes and GUI status
	Plugin *plugin;

// Storage of session parameters
	MenuEffectPrompt *prompt;
	int gui_on;

	VFrame *temp_frame;

// Icon for Asset Window
	VFrame *picon;

private:
	int reset_parameters();
	int cleanup_plugin();

// Base class created by client
	PluginClient *client;
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
	int use_opengl;
// Driver for opengl calls.
	VideoDevice *vdevice;
};

#endif

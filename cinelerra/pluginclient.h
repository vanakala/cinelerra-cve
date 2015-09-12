
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

#ifndef PLUGINCLIENT_H
#define PLUGINCLIENT_H

// Base class inherited by all the different types of plugins.

class PluginClient;

#include "aframe.inc"
#include "arraylist.h"
#include "autos.h"
#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "bchash.h"
#include "condition.h"
#include "edlsession.inc"
#include "keyframe.h"
#include "mainprogress.inc"
#include "pluginserver.inc"
#include "preferences.inc"
#include "theme.inc"
#include "vframe.h"

extern "C"
{
extern PluginClient* new_plugin(PluginServer *server);
}

class PluginClientAuto
{
public:
	float position;
	float intercept;
	float slope;
};

class PluginClient
{
public:
	PluginClient(PluginServer *server);
	virtual ~PluginClient();

// Queries for the plugin server.
	virtual int is_realtime() { return 0; };
	virtual int is_audio() { return 0; };
	virtual int is_video() { return 0; };
	virtual int is_theme() { return 0; };
	virtual int uses_gui() { return 1; };
	virtual int is_multichannel() { return 0; };
	virtual int is_synthesis() { return 0; };
	virtual int is_transition() { return 0; };
	virtual int has_pts_api() { return 0; };
// return the title of the plugin
	virtual const char* plugin_title();
	virtual VFrame* new_picon() { return 0; };
	virtual Theme* new_theme() { return 0; };
// Get theme being used by Cinelerra currently.  Used by all plugins.
	Theme* get_theme();

// Give the framerate of the output for a non realtime plugin.
// For realtime plugins give the requested framerate.
	virtual double get_framerate();

// get information from user before non realtime processing
	virtual int get_parameters() { return 0; };

	virtual void start_loop() {};
	virtual void stop_loop() {};

// Realtime commands for signal processors.
// These must be defined by the plugin itself.
// Set the string identifying the plugin to modules and patches.
	virtual void set_string() {};
// cause the plugin to show the gui
	virtual void show_gui() {};
// cause the plugin to hide the gui
	void client_side_close();
	void update_display_title();

// Raise the GUI
	virtual void raise_window() {};
	virtual void update_gui() {};
	virtual void save_data(KeyFrame *keyframe) {};    // write the plugin settings to text in text format
	virtual void read_data(KeyFrame *keyframe) {};    // read the plugin settings from the text
	int send_hide_gui();                                    // should be sent when the GUI recieves a close event from the user
	virtual void force_update() {};

	int get_configure_change();                             // get propogated configuration change from a send_configure_change

// Called by plugin server to update GUI with rendered data.
	virtual void plugin_render_gui(void *data) {};
	virtual void plugin_render_gui(void *data, int size) {};
	virtual int plugin_process_loop(VFrame **buffers) { return 1; };
	virtual int plugin_process_loop(AFrame **buffers) { return 1; };

// get parameters depending on video or audio
	virtual void init_realtime_parameters() {};
// release objects which are required after playback stops
	virtual void render_stop() {};
	char* get_gui_string();

// Used by themes
// Used by plugins which need to know where they are.
	char* get_path();

// Return keyframe objects.  The position in the resulting object 
// is relative to the EDL.
	KeyFrame* prev_keyframe_pts(ptstime postime);
	KeyFrame* next_keyframe_pts(ptstime postime);
	KeyFrame* first_keyframe();
// get current camera and projector position
	void get_camera(double *x, double *y, double *z, ptstime postime);
	void get_projector(double *x, double *y, double *z, ptstime postime);
// When this plugin is adjusted, propogate parameters back to EDL and virtual
// console.  This gets a keyframe from the EDL, with the position set to the
// EDL tracking position.
	void send_configure_change();

// Called from process_buffer
// Returns 1 if a GUI is open so OpenGL routines can determine if
// they can run.
	int gui_open();

// Get the EDL Session.  May return 0 if the server has no edl.
	EDLSession* get_edlsession();

// Plugin must call this before performing OpenGL operations.
// Returns 1 if the user supports opengl buffers.
	int get_use_opengl();

// Get total tracks to process
	int get_total_buffers();

// Get interpolation used by EDL from overlayframe.inc
	int get_interpolation_type();

// Get the values from the color picker
	float get_red();
	float get_green();
	float get_blue();

// Operations for file handlers
	virtual int open_file() { return 0; };
	virtual int get_audio_parameters() { return 0; };
	virtual int get_video_parameters() { return 0; };
	virtual int check_header(const char *path) { return 0; };
	virtual int open_file(const char *path, int wr, int rd) { return 1; };
	virtual int close_file() { return 0; };

// All plugins define these.
// load default settings for the plugin
	virtual void load_defaults() {};
// save the current settings as defaults
	virtual void save_defaults() {};
	virtual int load_configuration() { return 0; }

// Non realtime operations for signal processors.
	virtual void plugin_start_loop(ptstime start,
		ptstime end,
		int total_buffers);
	void plugin_stop_loop();

	MainProgressBar* start_progress(char *string, ptstime length);

// get framerate of EDL
	double get_project_framerate();
// Total number of processors - 1
	int get_project_smp();
	int get_aspect_ratio(float &aspect_w, float &aspect_h);

	virtual int plugin_get_parameters();
	void set_interactive();

// Load plugin defaults from file
	BC_Hash* load_defaults_file(const char *filename);
// Returns configuration directory of plugins
	const char *plugin_conf_dir();
// Abort plugin with a message
	void abort_plugin(const char *fmt, ...)
		__attribute__ ((__format__(__printf__, 2, 3)));

// Realtime operations.
	void plugin_init_realtime(int total_in_buffers);

// communication convenience routines for the base class
	int save_data_client();
	int load_data_client();
	void set_string_client(const char *string);  // set the string identifying the plugin
	int send_cancelled();        // non realtime plugin sends when cancelled

// ================================== Messages ===========================
	char gui_string[BCTEXTLEN];          // string identifying module and plugin

	int show_initially;             // set to show a realtime plugin initially
// range in project for processing
	ptstime start_pts, end_pts;
	int interactive;                // for the progress bar plugin
	int success;
	int total_in_buffers;           // total buffers allocated by the server

// Position relative to start of EDL.  Calculated for every process
// command.  Used for keyframes.
	ptstime source_pts;
// For realtime plugins gets the lowest pts (start of plugin)
	ptstime source_start_pts;

// Length of source.  For effects it's the plugin length.  For transitions
// it's the transition length.
	ptstime total_len_pts;

// Total number of processors available - 1
	int smp;
	PluginServer *server;
};

#endif

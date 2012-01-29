
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
#include "condition.h"
#include "edlsession.inc"
#include "keyframe.h"
#include "mainprogress.inc"
#include "maxbuffers.h"
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
	virtual int is_fileio() { return 0; };
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
	virtual int process_loop() { return 0; };
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

	int get_configure_change();                             // get propogated configuration change from a send_configure_change

// Called by plugin server to update GUI with rendered data.
	virtual void plugin_render_gui(void *data) {};
	virtual void plugin_render_gui(void *data, int size) {};
	virtual int plugin_process_loop(VFrame **buffers, int &write_length) { return 1; };
	virtual int plugin_process_loop(AFrame **buffers, int &write_length) { return 1; };

// get parameters depending on video or audio
	virtual void init_realtime_parameters() {};
// release objects which are required after playback stops
	virtual void render_stop() {};
	char* get_gui_string();

// Used by themes
// Used by plugins which need to know where they are.
	char* get_path();

// Return keyframe objects.  The position in the resulting object 
// is relative to the EDL rate.  This is the only way to avoid copying the
// data for every frame.
// If the result is the default keyframe, the keyframe's position is 0.
// position - relative to EDL rate or local rate to allow simple 
//     passing of get_source_position.
//     If -1 the tracking position in the edl is used.
// is_local - if 1, the position is converted to the EDL rate.
	KeyFrame* get_prev_keyframe(posnum position, int is_local = 1);
	KeyFrame* get_next_keyframe(posnum position, int is_local = 1);
	KeyFrame* prev_keyframe_pts(ptstime postime);
	KeyFrame* next_keyframe_pts(ptstime postime);
// get current camera and projector position
	void get_camera(float *x, float *y, float *z, framenum position);
	void get_projector(float *x, float *y, float *z, framenum position);
	void get_camera(float *x, float *y, float *z, ptstime postime);
	void get_projector(float *x, float *y, float *z, ptstime postime);
// When this plugin is adjusted, propogate parameters back to EDL and virtual
// console.  This gets a keyframe from the EDL, with the position set to the
// EDL tracking position.
	void send_configure_change();

// Called from process_buffer
// Returns 1 if a GUI is open so OpenGL routines can determine if
// they can run.
	int gui_open();

// Length of source.  For effects it's the plugin length.  For transitions
// it's the transition length.  Relative to the requested rate.
// The only way to get smooth interpolation is to make all position queries
// relative to the requested rate.
	posnum get_total_len();

// For realtime plugins gets the lowest sample of the plugin in the requested
// rate.  For others it's the start of the EDL selection in the EDL rate.
	posnum get_source_start();

// Convert the position relative to the requested rate to the position 
// relative to the EDL rate.  If the argument is < 0, it is not changed.
// Used for interpreting keyframes.
	virtual posnum local_to_edl(posnum position);

// Convert the EDL position to the local position.
	virtual posnum edl_to_local(posnum position);

// For transitions the source_position is the playback position relative
// to the start of the transition.
// For realtime effects, the start of the most recent process_buffer in forward
// and the end of the range to process in reverse.  Relative to start of EDL in
// the requested rate.
	posnum get_source_position();

// Get the EDL Session.  May return 0 if the server has no edl.
	EDLSession* get_edlsession();

// Get the direction of the most recent process_buffer
	int get_direction();

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
	virtual void plugin_start_loop(posnum start,
		posnum end,
		int buffer_size, 
		int total_buffers);
	virtual void plugin_start_loop(ptstime start,
		ptstime end,
		int total_buffers);
	void plugin_stop_loop();
	int plugin_process_loop();
	MainProgressBar* start_progress(char *string, int64_t length);
// get samplerate of EDL
	int get_project_samplerate();
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
	void reset();
	void plugin_init_realtime(int realtime_priority, 
		int total_in_buffers,
		int buffer_size);

// communication convenience routines for the base class
	int save_data_client();
	int load_data_client();
	void set_string_client(const char *string);  // set the string identifying the plugin
	int send_cancelled();        // non realtime plugin sends when cancelled

// ================================== Messages ===========================
	char gui_string[BCTEXTLEN];          // string identifying module and plugin

	int show_initially;             // set to show a realtime plugin initially
// range in project for processing
	posnum start, end;
	ptstime start_pts, end_pts;
	int interactive;                // for the progress bar plugin
	int success;
	int total_out_buffers;          // total send buffers allocated by the server
	int total_in_buffers;           // total recieve buffers allocated by the server
	int wr, rd;                     // File permissions for fileio plugins.

// These give the largest fragment the plugin is expected to handle.
// size of a send buffer to the server
	int out_buffer_size;
// size of a recieve buffer from the server
	int in_buffer_size;

// Direction of most recent process_buffer
	int direction;

// Operating system scheduling
	int realtime_priority;

// Position relative to start of EDL in requested rate.  Calculated for every process
// command.  Used for keyframes.
	posnum source_position;
	ptstime source_pts;
// For realtime plugins gets the lowest sample of the plugin in the requested
// rate.  For others it's always 0.
	posnum source_start;
	ptstime source_start_pts;
// Length of source.  For effects it's the plugin length.  For transitions
// it's the transition length.  Relative to the requested rate.
	posnum total_len;
	ptstime total_len_pts;

// Total number of processors available - 1
	int smp;
	PluginServer *server;
};

#endif

// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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
#include "keyframe.inc"
#include "menueffects.inc"
#include "plugin.inc"
#include "pluginserver.inc"
#include "pluginwindow.h"
#include "preferences.inc"
#include "theme.inc"
#include "trackrender.inc"
#include "vframe.h"

extern "C"
{
extern PluginClient* new_plugin(PluginServer *server);
}

class PluginClient
{
public:
	PluginClient(PluginServer *server);
	virtual ~PluginClient() {};

// Queries for the plugin server.
	virtual int is_realtime() { return 0; };
	virtual int is_audio() { return 0; };
	virtual int is_video() { return 0; };
	virtual int is_theme() { return 0; };
	virtual int uses_gui() { return 1; };
	virtual int is_multichannel() { return 0; };
	virtual int is_synthesis() { return 0; };
	virtual int is_transition() { return 0; };
	virtual int not_keyframeable() { return 0; };
// API version:
//   3 - plugin supports tmpframe, same instance can open gui
	virtual int api_version() { return 0; };
// plugin supporta OpenGL
	virtual int has_opengl_support() { return 0; }
// maximum number of channels supported by multichannel plugin
//  0 - unlimited number
	virtual int multi_max_channels() { return 0; }
// plugin shows status of the current frame
	virtual int has_status_gui() { return 0; }
// return the title of the plugin
	virtual const char* plugin_title();
	virtual VFrame* new_picon() { return 0; };
	virtual Theme* new_theme() { return 0; };

// get information from user before non realtime processing
	virtual int get_parameters() { return 0; };

	virtual void init_plugin() {};
// Free resources of idle plugin
	virtual void reset_plugin() {};

// Realtime commands for signal processors.
// These must be defined by the plugin itself.

// cause the plugin to show the gui
	virtual void show_gui() {};
	virtual void hide_gui() {};
// cause the plugin to hide the gui
	void client_side_close();
// Set pointer to menueffect window
	void set_prompt(MenuEffectPrompt *prompt);
	void update_display_title();

// Raise the GUI
	virtual void raise_window() {};
	virtual void update_gui() {};
	virtual void save_data(KeyFrame *keyframe) {};    // write the plugin settings to text in text format
	virtual void read_data(KeyFrame *keyframe) {};    // read the plugin settings from the text

	int get_configure_change();                             // get propogated configuration change from a send_configure_change

// Batch effects
	virtual int process_loop(AFrame **aframes) { return 1; };
	virtual int process_loop(VFrame **buffers) { return 1; };

// Transitions
	virtual void process_realtime(AFrame *input, AFrame *output) {};
	virtual void process_realtime(VFrame *input, VFrame *output) {};

// API v.3
	virtual void process_tmpframes(VFrame **frame) {};
	virtual VFrame *process_tmpframe(VFrame *frame) { return frame; };
	virtual void process_tmpframes(AFrame **frames) {};
	virtual AFrame *process_tmpframe(AFrame *frame) { return frame; };

// process the data in the buffers
// input - the current edit's data
// output - the previous edit's data and the destination of the transition output
// current_postime - Position from start of the transition and
//     relative to the transition.
	void process_transition(VFrame *input, VFrame *output,
		ptstime current_postime);
	void process_transition(AFrame *input, AFrame *output,
		ptstime current_postime);

// frame/buffer - video/audio fame to process
	void process_buffer(VFrame **frame);
	void process_buffer(AFrame **buffer);

	AFrame *get_frame(AFrame *frame);
	VFrame *get_frame(VFrame *buffer);

// Get, release tmpframes
	VFrame *clone_vframe(VFrame *orig);
	void release_vframe(VFrame *frame);
	AFrame *clone_aframe(AFrame *orig);
	void release_aframe(AFrame *frame);

// Get project samplerate
	int get_project_samplerate();
// Get framerate of project
	double get_project_framerate();
// Get video pixel aspect ratio
	double get_sample_aspect_ratio();
// Get color model of project
	int get_project_color_model();
// Get the start of the plugin
	ptstime get_start();
// Get the length of the plugin
	ptstime get_length();
// Get the end position of the plugin
	ptstime get_end();

	char* get_gui_string();

// Used by themes
// Used by plugins which need to know where they are.
	char* get_path();

// Return keyframe objects.
	KeyFrame* get_prev_keyframe(ptstime postime);
	KeyFrame* get_next_keyframe(ptstime postime);
	KeyFrame *get_first_keyframe();

	void set_keyframe(KeyFrame *keyframe);
	KeyFrame *get_keyframe();

// get current camera position
	void get_camera(double *x, double *y, double *z, ptstime postime);
// When this plugin is adjusted, propogate parameters back to EDL
	void send_configure_change();

// Plugin must call this before performing OpenGL operations.
// Returns 1 if the user supports opengl buffers.
	int get_use_opengl();

// Get total tracks to process
	int get_total_buffers();

// Get interpolation used by EDL from overlayframe.inc
	int get_interpolation_type();

// Get the values from the color picker
	void get_picker_colors(double *red, double *green, double *blue);
	void get_picker_rgb(int *red, int *green, int *blue);
	void get_picker_yuv(int *y, int *u, int *v);

// load default settings for the plugin
	virtual void load_defaults() {};
// save the current settings as defaults
	virtual void save_defaults() {};
	virtual int load_configuration() { return need_reconfigure; }

// Total number of processors
	int get_project_smp();

	int plugin_get_parameters(int channels);

	void plugin_show_gui();

// Load plugin defaults from file
	BC_Hash* load_defaults_file(const char *filename);
// Returns configuration directory of plugins
	const char *plugin_conf_dir();
// Abort plugin with a message
	void abort_plugin(const char *fmt, ...)
		__attribute__ ((__format__(__printf__, 2, 3)));
// Abort plugin with unsupported colormodel
	void unsupported(int cmodel);
// Initialize plugin
	void plugin_init(int total_in_buffers);

	virtual void render_gui(void *data) { };
// Sets the current trackrender (needed by get_?frame)
	void set_renderer(TrackRender *renderer);

// ================================== Messages ===========================
	char gui_string[BCTEXTLEN];          // string identifying module and plugin

	int success;
	int total_in_buffers;           // total buffers allocated by the server

// The currrent position relative to start of EDL.
	ptstime source_pts;

	int audio_buffer_size;

	PluginServer *server;
	MenuEffectPrompt *prompt;
// Plugin of EDL
	Plugin *plugin;
// Keyframe when plugin is not avalilable
	KeyFrame *keyframe;

protected:
	TrackRender *renderer;
	int need_reconfigure;
};

#endif

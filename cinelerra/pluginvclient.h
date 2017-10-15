
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

#ifndef PLUGINVCLIENT_H
#define PLUGINVCLIENT_H

#include "bcfontentry.h"
#include "cinelerra.h"
#include "pluginclient.h"
#include "vframe.inc"

#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>

// Maximum dimensions for a temporary frame a plugin should retain between 
// process_buffer calls.  This allows memory conservation.
#define PLUGIN_MAX_W 2000
#define PLUGIN_MAX_H 1000

class PluginVClient : public PluginClient
{
public:
	PluginVClient(PluginServer *server);
	virtual ~PluginVClient();

	void init_realtime_parameters();
	int is_video();

// Multichannel buffer process for backwards compatibility
	virtual void process_realtime(VFrame **input, 
		VFrame **output) {};
// Single channel buffer process for backwards compatibility and transitions
	virtual void process_realtime(VFrame *input, 
		VFrame *output) {};

// Process buffer using pull method.  By default this loads the input into *frame
//     and calls process_realtime with input and output pointing to frame.
	virtual void process_frame(VFrame **frame);
	virtual void process_frame(VFrame *frame);

// Called by plugin server to render the GUI with rendered data.
	void plugin_render_gui(void *data);
	virtual void render_gui(void *data) { };
// Called by client to cause GUI to be rendered with data.
	void send_render_gui(void *data);
	virtual int process_loop(VFrame **buffers) { return 1; };
	virtual int process_loop(VFrame *buffer) { return 1; };
	int plugin_process_loop(VFrame **buffers);
	int plugin_get_parameters();

	void get_frame(VFrame *buffer, int use_opengl = 0);

// User calls this to request an opengl routine to be run synchronously.
	void run_opengl();

// Called by Playback3D to run opengl commands synchronously.
// Overridden by the user with the commands to run synchronously.
	virtual void handle_opengl() {};

// Used by the opengl handlers to get the 
// arguments to process_buffer.
// For realtime plugins, they're identical for input and output.
	VFrame* get_input(int channel = 0);
	VFrame* get_output(int channel = 0);

// Called by user to allocate the temporary for the current process_buffer.  
// It may be deleted after the process_buffer to conserve memory.
	VFrame* new_temp(int w, int h, int color_model);
// Called by PluginServer after process_buffer to delete the temp if it's too
// large.
	void age_temp();
	VFrame* get_temp();

// Frame rate relative to EDL
	double get_project_framerate();
// Frame rate requested
	double get_framerate();
// Get list of system fonts
	ArrayList<BC_FontEntry*> *get_fontlist();
// Find font entry
	BC_FontEntry *find_fontentry(const char *displayname, int style,
		int mask, int preferred_style);
// Find font path where glyph for the character_code exists
	int find_font_by_char(FT_ULong char_code, char *path_new, const FT_Face oldface);

// ======================== Realtime buffer pointers ===========================
// These are provided by the plugin server for the opengl handler.
	VFrame *input[MAXCHANNELS];
	VFrame *output[MAXCHANNELS];

// Frame rate of EDL
	double project_frame_rate;
// Local parameters set by non realtime plugin about the file to be generated.
// Retrieved by server to set output file format.
// In realtime plugins, these are set before every process_buffer as the
// requested rates.
	double frame_rate;
	int project_color_model;

// Aspect ratio
	double aspect_w;
	double aspect_h;

// Tempo
	VFrame *temp;
};

#endif

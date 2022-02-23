// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>


#ifndef PLUGIN_H
#define PLUGIN_H

#include "aframe.inc"
#include "edl.inc"
#include "filexml.inc"
#include "guidelines.h"
#include "keyframe.inc"
#include "keyframes.inc"
#include "plugin.inc"
#include "pluginpopup.inc"
#include "pluginclient.inc"
#include "pluginserver.inc"
#include "trackplugin.inc"
#include "track.inc"
#include "vframe.inc"

class Plugin
{
public:
	Plugin(EDL *edl, Track *track, PluginServer *server);
	~Plugin();

// Called by Edits::equivalent_output
	void equivalent_output(Plugin *plugin, ptstime *result);

	int is_synthesis();

	void copy(Plugin *plugin, ptstime start, ptstime end);
	void copy_from(Plugin *plugin);

	int identical(Plugin *that);

// Shift plugin keyframes
	void shift_keyframes(ptstime difference);
// Remove keyframes after pts
	void remove_keyframes_after(ptstime pts);

	void change_plugin(PluginServer *server, int plugin_type,
		Plugin *shared_plugin, Track *shared_track);
// For synchronizing parameters
	void copy_keyframes(Plugin *plugin);

	void save_xml(FileXML *file);
	void save_shared_location(FileXML *file);
	void calculate_ref_numbers();
	void calculate_ref_ids();
	void init_pointers_by_ids();
	int module_number();
	void load(FileXML *file, ptstime start = 0);
	void init_shared_pointers();
// Shift in time
	void shift(ptstime difference);
	void dump(int indent = 0);
// Called by PluginClient sequence to get rendering parameters
	KeyFrame* get_prev_keyframe(ptstime postime);
	KeyFrame* get_next_keyframe(ptstime postime);
// Get keyframes for editing with automatic creation if enabled.
	KeyFrame* get_keyframe(ptstime selpos);
	int silence();
// Calculate title given plugin type.  Used by TrackCanvas::draw_plugins
	void calculate_title(char *string);
// Remove all keyframes
	void clear_keyframes();
// Get camera coordinates at postion
	void get_camera(double *x, double *y, double *z, ptstime postime);
// Position, length
	inline ptstime get_pts() { return pts; };
	inline ptstime get_length() { return duration; };
	inline ptstime end_pts() { return pts + duration; };
	void set_pts(ptstime pts);
	void set_length(ptstime length);
	void set_end(ptstime end);
	Plugin *active_in(ptstime start, ptstime end);
// Client
	void update_plugin_gui();
	void update_display_title();
	void hide_plugin_gui();
	int show_plugin_gui();
	void update_toggles();
	void reset_plugin();

	ptstime plugin_change_duration(ptstime start, ptstime length);
	int get_number();
	int get_multichannel_number();
	size_t get_size();
	int shared_slots();

	int id;

	int plugin_type;
	int show, on;
	int idle;
	Track *track;
	EDL *edl;

	KeyFrames *keyframes;
// Shared track of type sharedmodule
	Track *shared_track;
// Shared plugin of type sharedplugin
	Plugin *shared_plugin;
// Guidelines of plugin
	GuideFrame *guideframe;
// Server
	PluginServer *plugin_server;
// Client
	PluginClient *client;
// Apiversion of client
	int apiversion;
// TrackPlugin of the plugin
	TrackPlugin *trackplugin;

private:
	int shared_track_num;
	int shared_plugin_num;
	int shared_track_id;
	int shared_plugin_id;
	ptstime pts;
	ptstime duration;
};

#endif

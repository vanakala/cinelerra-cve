
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

#ifndef PLUGIN_H
#define PLUGIN_H

#include "edl.inc"
#include "filexml.inc"
#include "guidelines.h"
#include "keyframe.inc"
#include "keyframes.inc"
#include "module.inc"
#include "plugin.inc"
#include "pluginpopup.inc"
#include "pluginserver.inc"
#include "track.inc"
#include "virtualnode.inc"

class PluginOnToggle;


class Plugin
{
public:
	Plugin(EDL *edl, Track *track, PluginServer *server);
	~Plugin();

// Called by Edits::equivalent_output to override the keyframe behavior and check
// title.
	void equivalent_output(Plugin *plugin, ptstime *result);

// Called by playable tracks to test for playable server.
// Descends the plugin tree without creating a virtual console.
	int is_synthesis();

	void copy(Plugin *plugin, ptstime start, ptstime end);
	void copy_from(Plugin *plugin);

	int identical(Plugin *that);
// Called by render_gui.  Only need the track, position,
// to determine a corresponding GUI.
	int identical_location(Plugin *that);
	void synchronize_params(Plugin *edit);
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
	KeyFrame* first_keyframe();
// Get keyframes for editing with automatic creation if enabled.
// The direction is always assumed to be forward.
	virtual KeyFrame* get_keyframe();
	int silence();
// Calculate title given plugin type.  Used by TrackCanvas::draw_plugins
	void calculate_title(char *string, int use_nudge);

// Position, length
	ptstime get_pts();
	ptstime get_length();
	ptstime end_pts();
	void set_pts(ptstime pts);
	void set_length(ptstime length);
	void set_end(ptstime end);
	Plugin *active_in(ptstime start, ptstime end);

	ptstime plugin_change_duration(ptstime start, ptstime length);
	int get_number();
	size_t get_size();

	int id;

	int plugin_type;
	int show, on;
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

private:
	int shared_track_num;
	int shared_plugin_num;
	int shared_track_id;
	int shared_plugin_id;
	ptstime pts;
	ptstime duration;
};

#endif

// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef TRACK_H
#define TRACK_H

#include "arraylist.h"
#include "asset.inc"
#include "automation.inc"
#include "bcwindowbase.inc"
#include "datatype.h"
#include "bchash.inc"
#include "edit.inc"
#include "edits.inc"
#include "edl.inc"
#include "filexml.inc"
#include "floatautos.inc"
#include "keyframe.inc"
#include "linklist.h"
#include "plugin.inc"
#include "pluginserver.inc"
#include "theme.inc"
#include "trackrender.inc"
#include "tracks.inc"

#include <stdint.h>

class Track : public ListItem<Track>
{
public:
	Track(EDL *edl, Tracks *tracks);
	virtual ~Track();

	int get_id();
	void load(FileXML *file);
	void load_pluginset(FileXML *file, ptstime start);
	void init_shared_pointers();
	virtual void save_header(FileXML *file) {};
	virtual void set_default_title() {};
	void equivalent_output(Track *track, ptstime *result);

	virtual void copy_from(Track *track);
	Track& operator=(Track& track);

// Get number of pixels to display
	virtual int vertical_span(Theme *theme);
	int get_canvas_number(Plugin *plugin);

// Get length of track in seconds
	ptstime get_length();
// Get length of effects in seconds
	ptstime get_effects_length(int is_synthesis);
// Get dimensions of source for convenience functions
	void get_source_dimensions(ptstime position, int &w, int &h);

// Editing
	void insert_asset(Asset *asset, int stream, int channel,
		ptstime length, ptstime position,
		int overwrite = 0);
	Plugin* insert_effect(PluginServer *server,
		ptstime start,
		ptstime length,
		int plugin_type,
		Plugin *shared_plugin,
		Track *shared_track);
	void insert_plugin(Track *track, ptstime position,
		ptstime length = -1, int overwrite = 0);
	Edit *editof(ptstime postime);
// Insert a track from another EDL
	void insert_track(Track *track, ptstime length,
		ptstime position, int overwrite = 0);

	void move_plugin_up(Plugin *plugin);
	void move_plugin_down(Plugin *plugin);
	void remove_plugin(Plugin *plugin);
	void reset_plugins(ptstime pts);
	void reset_renderers();
	void remove_asset(Asset *asset);
	void detach_transition(Plugin *transition);
	void cleanup();
	// Maximum possible plugin start pts on track
	ptstime plugin_max_start(Plugin *plugin);

	void update_plugin_guis();
	void update_plugin_titles();

// Used for determining a selection for editing so leave as int.
// converts the selection to SAMPLES OR FRAMES and stores in value
	virtual posnum to_units(ptstime position, int round = 0);
// For drawing
	virtual ptstime from_units(posnum position);

// detach shared effects referencing plugin or track
	void detach_shared_effects(Plugin *plugin, Track *track);

// Called by playable tracks to test for playable server.
// Descends the plugin tree without creating a virtual console.
// Used by PlayableTracks::is_playable.
	int is_synthesis(ptstime position);

	void copy_settings(Track *track);
	void shift_keyframes(ptstime position, ptstime length);
	void shift_effects(ptstime position, ptstime length);

	EDL *edl;
	Tracks *tracks;

	Edits *edits;
	ArrayList<Plugin*> plugins;
	Automation *automation;

	TrackRender *renderer;

// Vertical offset from top of timeline
	int y_pixel;
	int expand_view;
	int draw;
// There is some debate on whether to expand gang from faders to
// dragging operations.  This would allow every edit in a column to get dragged
// simultaneously.
	int gang;
	char title[BCTEXTLEN];
	int play;
	int record;
// This track is master
	int master;
// Nudge in seconds.  Positive shifts track earlier in time.  This way
// the position variables only need to add the nudge.
	ptstime nudge;
// TRACK_AUDIO or TRACK_VIDEO
	int data_type;

	int load_automation(FileXML *file);
	int load_edits(FileXML *file);

	void dump(int indent = 0);

// ===================================== editing
	void save_xml(FileXML *file, const char *output_path = "");
	void copy(Track *track, ptstime start, ptstime end);
	void copy_assets(ptstime start,
		ptstime end,
		ArrayList<Asset*> *asset_list);
	void clear(ptstime start, ptstime end);
	void clear_plugins(ptstime start, ptstime end);
// remove everything after pts
	void clear_after(ptstime pts);

	void clear_automation(ptstime selectionstart,
		ptstime selectionend);
	void straighten_automation(ptstime selectionstart,
		ptstime selectionend);
	void automation_xml(FileXML *file);
	void load_effects(FileXML *file, int operation);
	void load_pluginsets(FileXML *file, int operation);
	void clear_handle(ptstime start,
		ptstime end,
		int actions,
		ptstime &distance);
	void paste_silence(ptstime start, ptstime end);

	ptstime adjust_position(ptstime oldposition, ptstime newposition,
		int currentend, int handle_mode);

	void modify_edithandles(ptstime oldposition,
		ptstime newposition,
		int currentend, 
		int handle_mode);

// Absolute number of this track
	int number_of();
// Longest time from current_position in which nothing changes
	ptstime edit_change_duration(ptstime input_position, 
		ptstime input_length, 
		int test_transitions);
	ptstime plugin_change_duration(ptstime input_position,
		ptstime input_length);
// Utility for edit_change_duration.
	int need_edit(Edit *current, int test_transitions);
// If the edit under position is playable.
// Used by PlayableTracks::is_playable.
	int playable_edit(ptstime position);
// Get shared track
	Plugin *get_shared_track(ptstime start, ptstime end);
// Get shared multichannel plugin
	Plugin *get_shared_multichannel(ptstime start, ptstime end);
// Number of bytes used
	size_t get_size();
// Return track type as STRDSC
	int get_strdsc();
// Check data_type against strdsc
	int is_strdsc(int strdsc);

// Dimensions of this track if video
	int track_w, track_h;
// Length of one unit on seconds
	ptstime one_unit;
private:
// Identification of the track
	int id;
};

#endif

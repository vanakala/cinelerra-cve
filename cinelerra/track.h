
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
#include "pluginset.inc"
#include "renderengine.inc"
#include "theme.inc"
#include "tracks.inc"

#include <stdint.h>

class Track : public ListItem<Track>
{
public:
	Track(EDL *edl, Tracks *tracks);
	virtual ~Track();

	int get_id();
	void load(FileXML *file, int track_offset, uint32_t load_flags);
	void init_shared_pointers();
	virtual void save_header(FileXML *file) {};
	void equivalent_output(Track *track, ptstime *result);

	virtual void copy_from(Track *track);
	Track& operator=(Track& track);
// Synchronize playback numbers
	void synchronize_params(Track *track);

// Get number of pixels to display
	virtual int vertical_span(Theme *theme);

// Get length of track in seconds
	ptstime get_length();
// Get dimensions of source for convenience functions
	void get_source_dimensions(ptstime position, int &w, int &h);

// Editing
	void insert_asset(Asset *asset, 
		ptstime length,
		ptstime position,
		int track_number,
		int overwrite = 0);
	Plugin* insert_effect(const char *title, 
		ptstime start,
		ptstime length,
		int plugin_type,
		Plugin *shared_plugin,
		Track *shared_track);
	void insert_plugin_set(Track *track, ptstime position,
		ptstime length = -1, int overwrite = 0);
	void detach_effect(Plugin *plugin);
// Insert a track from another EDL
	void insert_track(Track *track, ptstime length,
		ptstime position, int overwrite = 0);
// Optimize editing
	void optimize();

	void xchg_pluginsets(PluginSet *set1, PluginSet *set2);
	void move_plugins_up(PluginSet *plugin_set);
	void move_plugins_down(PluginSet *plugin_set);
	void remove_pluginset(PluginSet *plugin_set);
	void remove_asset(Asset *asset);

// Used for determining a selection for editing so leave as int.
// converts the selection to SAMPLES OR FRAMES and stores in value
	virtual posnum to_units(ptstime position, int round = 0);
// For drawing
	virtual ptstime from_units(posnum position);

// Positions are identical for handle modifications
	int identical(ptstime sample1, ptstime sample2);

// Get the plugin belonging to the set.
	Plugin* get_current_plugin(ptstime postime,
		int plugin_set, 
		int use_nudge);
	Plugin* get_current_transition(ptstime position);

// detach shared effects referencing plugin or track
	void detach_shared_effects(Plugin *plugin, Track *track);

// Called by playable tracks to test for playable server.
// Descends the plugin tree without creating a virtual console.
// Used by PlayableTracks::is_playable.
	int is_synthesis(RenderEngine *renderengine, 
		ptstime position);

// Used by PlayableTracks::is_playable
// Returns 1 if the track is in the output boundaries.
	virtual int is_playable(ptstime position);

// Used by PlayableTracks::is_playable
	int plugin_used(ptstime position);

	void copy_settings(Track *track);
	void shift_keyframes(ptstime position, ptstime length);
	void shift_effects(ptstime position, ptstime length);

	EDL *edl;
	Tracks *tracks;

	Edits *edits;
// Plugin set uses key frames for automation
	ArrayList<PluginSet*> plugin_set;
	Automation *automation;

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
	void clear(ptstime start,
		ptstime end,
		int actions);
// remove everything after pts
	void clear_after(ptstime pts);
// Returns the point to restart background rendering at.
// -1 means nothing changed.
	void clear_automation(double selectionstart, 
		double selectionend, 
		int shift_autos,
		int default_only);
	void straighten_automation(double selectionstart, 
		double selectionend);
	void automation_xml(FileXML *file);
	void paste_automation(ptstime selectionstart, 
		ptstime total_length, 
		double frame_rate,
		int sample_rate,
		FileXML *file);
	void clear_handle(ptstime start,
		ptstime end,
		int actions,
		ptstime &distance);
	void paste_silence(ptstime start, ptstime end, int edit_plugins);

	ptstime adjust_position(ptstime oldposition, ptstime newposition,
		int currentend, int handle_mode);

	void modify_edithandles(ptstime oldposition,
		ptstime newposition,
		int currentend, 
		int handle_mode);
	void modify_pluginhandles(ptstime oldposition,
		ptstime newposition,
		int currentend, 
		int handle_mode,
		int edit_labels);

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

// Dimensions of this track if video
	int track_w, track_h;
// Length of one unit on seconds
	ptstime one_unit;
private:
// Identification of the track
	int id;
};

#endif

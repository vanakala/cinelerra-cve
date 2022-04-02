// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef TRACKS_H
#define TRACKS_H

#include "asset.inc"
#include "autoconf.h"
#include "edl.inc"
#include "file.inc"
#include "filexml.inc"
#include "linklist.h"
#include "pluginserver.inc"
#include "track.h"
#include "trackcanvas.inc"


class Tracks : public List<Track>
{
public:
	Tracks(EDL *edl);
	~Tracks();

	void reset_instance();
	void reset_plugins();
	void reset_renderers();
	Tracks& operator=(Tracks &tracks);
	void load(FileXML *xml);
	void init_shared_pointers();

	void move_effect(Plugin *plugin,
		Track *track, 
		ptstime position);

// Construct a list of all the recordable edits which start on position
	void get_affected_edits(ArrayList<Edit*> *drag_edits, 
		ptstime position,
		Track *start_track);

	void get_automation_extents(double *min,
		double *max,
		ptstime start,
		ptstime end,
		int autogrouptype);

	void equivalent_output(Tracks *tracks, ptstime *result);
	void move_track_up(Track *track);        // move recordable tracks up
	void move_track_down(Track *track);      // move recordable tracks down
	void move_tracks_up();                    // move recordable tracks up
	void move_tracks_down();                  // move recordable tracks down

// Return the numbers of tracks with the play patch enabled
	int playable_tracks_of(int type);
// Return number of tracks with the record patch enabled
	int recordable_tracks_of(int type);
	int total_tracks_of(int type);
	ptstime total_length_of(int type);
	ptstime duration_framealigned(double fps);
	ptstime duration();
// Update y pixels after a zoom
	void update_y_pixels(Theme *theme);

// add a track
	Track* add_track(int track_type, int above, Track *dst_track);
// delete any track
	void delete_track(Track* track);
// detach shared effects referencing track or plugin
	void detach_shared_effects(Plugin *plugin, Track *track);
// Append asset to existing tracks, returns inserted length
	ptstime append_asset(Asset *asset, ptstime paste_at = -1,
		Track *first_track = 0, int overwrite = 0);
// Append tracks to existing tracks
	ptstime append_tracks(Tracks *tracks, ptstime paste_at = -1,
		Track *first_track = 0, int overwrite = 0);
// Init pointers of shared plugins from
	void init_plugin_pointers_by_ids();
// Create new tracks and insert asset
	void create_new_tracks(Asset *asset);
// Create new tracks and insert tracks
	void create_new_tracks(Tracks *tracks);
// Cleanup over all tracks
	void cleanup();
// Cleanup of multichannel shared plugins
	void cleanup_plugins();
// Check if plugin is shared with track
	int shared_on_track(Plugin *plugin, Track *track);
// Number of bytes used
	size_t get_size();

	EDL *edl;

// Types for drag toggle behavior
	enum
	{
		NONE,
		PLAY,
		RECORD,
		GANG,
		DRAW,
		MUTE,
		EXPAND,
		MASTER
	};

	void dump(int indent = 0);

// Append all the tracks to the end of the recordable tracks
	void concatenate_tracks();

	int delete_tracks(void);     // delete all the recordable tracks
	void delete_all_tracks();      // delete just the tracks
	ptstime paste_duration(ptstime position, Asset *asset,
		Track *first_track, ptstime prev_duration);
	ptstime paste_duration(ptstime position, EDL *clip,
		Track *first_track, ptstime prev_duration);

	void copy_from(Tracks *tracks);
// Get master track
	Track *master();

// ================================== EDL editing
	void save_xml(FileXML *file, const char *output_path = "");

	void copy(Tracks *tracks, ptstime start, ptstime end,
		ArrayList<Track*> *src_tracks = 0);

	void clear(ptstime start, ptstime end);
	void clear_automation(ptstime selectionstart,
		ptstime selectionend);
	void straighten_automation(ptstime selectionstart,
		ptstime selectionend);
	void clear_handle(ptstime start,
		ptstime end,
		double &longest_distance,
		int actions);
	void automation_xml(FileXML *file);
// Loads effects from xml to empty Tracks
	void load_effects(FileXML *file, int operation);
	void paste_silence(ptstime start,
		ptstime end);

	ptstime adjust_position(ptstime oldposition, ptstime newposition,
		int currentend, int handle_mode);

	void modify_edithandles(ptstime oldpodition,
		ptstime newposition,
		int currentend, 
		int handle_mode);

	ptstime total_length();     // Longest track.

	int totalpixels();       // height of all tracks in pixels

private:
};

#endif

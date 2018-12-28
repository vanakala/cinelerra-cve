
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

#ifndef TRACKS_H
#define TRACKS_H

#include "autoconf.h"
#include "edl.inc"
#include "file.inc"
#include "filexml.inc"
#include "linklist.h"
#include "pluginserver.inc"
#include "track.h"
#include "trackcanvas.inc"
#include "transition.inc"


class Tracks : public List<Track>
{
public:
	Tracks();
	Tracks(EDL *edl);
	virtual ~Tracks();

	Tracks& operator=(Tracks &tracks);
	void load(FileXML *xml, int &track_offset, uint32_t load_flags);
	void move_edits(ArrayList<Edit*> *edits, 
		Track *track,
		ptstime position,
		int behaviour);
	void move_effect(Plugin *plugin,
		PluginSet *plugin_set,
		Track *track, 
		ptstime position);

// Construct a list of all the recordable edits which start on position
	void get_affected_edits(ArrayList<Edit*> *drag_edits, 
		ptstime position,
		Track *start_track);

	void get_automation_extents(float *min, 
		float *max,
		ptstime start,
		ptstime end,
		int autogrouptype);

	void equivalent_output(Tracks *tracks, ptstime *result);

	void move_track_up(Track *track);        // move recordable tracks up
	void move_track_down(Track *track);      // move recordable tracks down
	int move_tracks_up();                    // move recordable tracks up
	int move_tracks_down();                  // move recordable tracks down
	void paste_audio_transition(PluginServer *server);
	void paste_video_transition(PluginServer *server, int first_track = 0);

	void paste_transition(PluginServer *server, Edit *dest_edit);
// Return the numbers of tracks with the play patch enabled
	int playable_audio_tracks();
	int playable_video_tracks();
// Return number of tracks with the record patch enabled
	int recordable_audio_tracks();
	int recordable_video_tracks();
	int total_audio_tracks();
	int total_video_tracks();
	ptstime total_audio_length();
	ptstime total_video_length();
	ptstime total_length_framealigned(double fps);
// Update y pixels after a zoom
	void update_y_pixels(Theme *theme);
// Total number of tracks where the following toggles are selected
	void select_all(int type, int value);
	void translate_projector(float offset_x, float offset_y);
	int total_of(int type);
// add a track
	Track* add_audio_track(int above, Track *dst_track);
	Track* add_video_track(int above, Track *dst_track);
// delete any track
	void delete_track(Track* track);
// detach shared effects referencing module
	void detach_shared_effects(int module);

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
		EXPAND
	};

	void change_channels(int oldchannels, int newchannels);
	void dump(int indent = 0);

// Change references to shared modules in all tracks from old to new.
// If do_swap is true values of new are replaced with old.
	void change_modules(int old_location, int new_location, int do_swap);
// Append all the tracks to the end of the recordable tracks
	int concatenate_tracks(int edit_plugins);
// Change references to shared plugins in all tracks
	void change_plugins(SharedLocation &old_location, SharedLocation &new_location, int do_swap);
	int delete_tracks(void);     // delete all the recordable tracks
	void delete_all_tracks();      // delete just the tracks

	void copy_from(Tracks *tracks);

// ================================== EDL editing
	void copy(ptstime start,
		ptstime end,
		int all, 
		FileXML *file, 
		const char *output_path = "");

	int copy_assets(FileXML *xml, 
		double start, 
		double end, 
		int all);
	void clear(ptstime start, ptstime end, int clear_plugins);
	void clear_automation(ptstime selectionstart,
		ptstime selectionend);
	void straighten_automation(ptstime selectionstart,
		ptstime selectionend);
	void clear_default_keyframe(void);
	void clear_handle(ptstime start,
		ptstime end,
		double &longest_distance,
		int actions);
	void copy_automation(ptstime selectionstart,
		ptstime selectionend,
		FileXML *file,
		int default_only,
		int autos_only);
	void copy_default_keyframe(FileXML *file);
	void paste_automation(ptstime selectionstart,
		FileXML *xml,
		int default_only);
	void paste_default_keyframe(FileXML *file);
	void paste_silence(ptstime start,
		ptstime end,
		int edit_plugins);

	void modify_edithandles(ptstime &oldposition,
		ptstime &newposition,
		int currentend, 
		int handle_mode,
		int actions);
	void modify_pluginhandles(ptstime &oldposition,
		ptstime &newposition,
		int currentend, 
		int handle_mode,
		int edit_labels);

	ptstime total_playable_length();     // Longest track.
// Used by equivalent_output
	int total_playable_vtracks();
	ptstime total_recordable_length();   // Longest track with recording on
	int totalpixels();       // height of all tracks in pixels
	int number_of(Track *track);        // track number of pointer
	Track* number(int number);      // pointer to track number


private:
};

#endif

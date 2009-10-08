
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
#include "autoconf.inc"
#include "automation.inc"
#include "datatype.h"
#include "bchash.inc"
#include "edit.inc"
#include "edits.inc"
#include "edl.inc"
#include "filexml.inc"
#include "floatautos.inc"
#include "guicast.h"
#include "keyframe.inc"
#include "linklist.h"
#include "module.inc"
#include "patch.inc"
#include "plugin.inc"
#include "pluginset.inc"
#include "renderengine.inc"
#include "sharedlocation.inc"
#include "theme.inc"
#include "intautos.inc"
#include "trackcanvas.inc"
#include "tracks.inc"
#include "transition.inc"

#include <stdint.h>

// UNITS ARE SAMPLES FOR ALL

class Track : public ListItem<Track>
{
public:
	Track(EDL *edl, Tracks *tracks);
	Track();
	virtual ~Track();

	int create_objects();
	int get_id();
	virtual int load_defaults(BC_Hash *defaults);
	int load(FileXML *file, int track_offset, uint32_t load_flags);
	virtual int save_header(FileXML *file) { return 0; };
	virtual int save_derived(FileXML *file) { return 0; };
	virtual int load_header(FileXML *file, uint32_t load_flags) { return 0; };
	virtual int load_derived(FileXML *file, uint32_t load_flags) { return 0; };
	void equivalent_output(Track *track, double *result);

	virtual void copy_from(Track *track);
	Track& operator=(Track& track);
	virtual PluginSet* new_plugins() { return 0; };
// Synchronize playback numbers
	virtual void synchronize_params(Track *track);

// Get number of pixels to display
	virtual int vertical_span(Theme *theme);
	int64_t horizontal_span();
	void resample(double old_rate, double new_rate);

// Get length of track in seconds
	double get_length();
// Get dimensions of source for convenience functions
	void get_source_dimensions(double position, int &w, int &h);

// Editing
	void insert_asset(Asset *asset, 
		double length, 
		double position, 
		int track_number);
	Plugin* insert_effect(char *title, 
		SharedLocation *shared_location, 
		KeyFrame *keyframe,
		PluginSet *plugin_set,
		double start,
		double length,
		int plugin_type);
	void insert_plugin_set(Track *track, double position);
	void detach_effect(Plugin *plugin);
// Insert a track from another EDL
	void insert_track(Track *track, 
		double position, 
		int replace_default,
		int edit_plugins);
// Optimize editing
	void optimize();
	int is_muted(int64_t position, int direction);  // Test muting status

	void move_plugins_up(PluginSet *plugin_set);
	void move_plugins_down(PluginSet *plugin_set);
	void remove_pluginset(PluginSet *plugin_set);
	void remove_asset(Asset *asset);

// Used for determining a selection for editing so leave as int.
// converts the selection to SAMPLES OR FRAMES and stores in value
	virtual int64_t to_units(double position, int round);
// For drawing
	virtual double to_doubleunits(double position);
	virtual double from_units(int64_t position);



// Positions are identical for handle modifications
    virtual int identical(int64_t sample1, int64_t sample2) { return 0; };

// Get the plugin belonging to the set.
	Plugin* get_current_plugin(double position, 
		int plugin_set, 
		int direction, 
		int convert_units,
		int use_nudge);
	Plugin* get_current_transition(double position, 
		int direction, 
		int convert_units,
		int use_nudge);

// detach shared effects referencing module
	void detach_shared_effects(int module);	


// Called by playable tracks to test for playable server.
// Descends the plugin tree without creating a virtual console.
// Used by PlayableTracks::is_playable.
	int is_synthesis(RenderEngine *renderengine, 
		int64_t position, 
		int direction);

// Used by PlayableTracks::is_playable
// Returns 1 if the track is in the output boundaries.
	virtual int is_playable(int64_t position, 
		int direction);

// Test direct copy conditions common to all the rendering routines
	virtual int direct_copy_possible(int64_t start, int direction, int use_nudge);

// Used by PlayableTracks::is_playable
	int plugin_used(int64_t position, int64_t direction);





	virtual int copy_settings(Track *track);
	void shift_keyframes(double position, double length, int convert_units);
	void shift_effects(double position, double length, int convert_units);
	void change_plugins(SharedLocation &old_location, 
		SharedLocation &new_location, 
		int do_swap);
	void change_modules(int old_location, 
		int new_location, 
		int do_swap);

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
// Nudge in track units.  Positive shifts track earlier in time.  This way
// the position variables only need to add the nudge.
	int64_t nudge;
// TRACK_AUDIO or TRACK_VIDEO
	int data_type;     



















	int load_automation(FileXML *file);
	int load_edits(FileXML *file);

	virtual int change_channels(int oldchannels, int newchannels) { return 0; };
	virtual int dump();



// ===================================== editing
	int copy(double start, 
		double end, 
		FileXML *file, 
		char *output_path = "");
	int copy_assets(double start, 
		double end, 
		ArrayList<Asset*> *asset_list);
	virtual int copy_derived(int64_t start, int64_t end, FileXML *file) { return 0; };
	virtual int paste_derived(int64_t start, int64_t end, int64_t total_length, FileXML *file, int &current_channel) { return 0; };
	int clear(double start, 
		double end, 
		int edit_edits,
		int edit_labels,
		int clear_plugins, 
		int convert_units,
		Edits *trim_edits);
// Returns the point to restart background rendering at.
// -1 means nothing changed.
	void clear_automation(double selectionstart, 
		double selectionend, 
		int shift_autos   /* = 1 */,
		int default_only  /* = 0 */);
	void straighten_automation(double selectionstart, 
		double selectionend);
	virtual int clear_automation_derived(AutoConf *auto_conf, 
		double selectionstart, 
		double selectionend, 
		int shift_autos = 1) { return 0; };
	virtual int clear_derived(double start, 
		double end) { return 0; };

	int copy_automation(double selectionstart, 
		double selectionend, 
		FileXML *file,
		int default_only,
		int autos_only);
	virtual int copy_automation_derived(AutoConf *auto_conf, 
		double selectionstart, 
		double selectionend, 
		FileXML *file) { return 0; };
	int paste_automation(double selectionstart, 
		double total_length, 
		double frame_rate,
		int64_t sample_rate,
		FileXML *file,
		int default_only);
	virtual int paste_automation_derived(double selectionstart, 
		double selectionend, 
		double total_length, 
		FileXML *file, 
		int shift_autos, 
		int &current_pan) { return 0; };
	int paste_auto_silence(double start, double end);
	virtual int paste_auto_silence_derived(int64_t start, int64_t end) { return 0; };
	int scale_time(float rate_scale, int scale_edits, int scale_autos, int64_t start, int64_t end);
	virtual int scale_time_derived(float rate_scale, int scale_edits, int scale_autos, int64_t start, int64_t end) { return 0; };
	int purge_asset(Asset *asset);
	int asset_used(Asset *asset);
	int clear_handle(double start, 
		double end, 
		int clear_labels,
		int clear_plugins, 
		double &distance);
	int paste_silence(double start, double end, int edit_plugins);
	virtual int select_translation(int cursor_x, int cursor_y) { return 0; };  // select video coordinates for frame
	virtual int update_translation(int cursor_x, int cursor_y, int shift_down) { return 0; };  // move video coordinates
	int select_auto(AutoConf *auto_conf, int cursor_x, int cursor_y);
	virtual int select_auto_derived(float zoom_units, float view_start, AutoConf *auto_conf, int cursor_x, int cursor_y) { return 0; };
	int move_auto(AutoConf *auto_conf, int cursor_x, int cursor_y, int shift_down);
	virtual int move_auto_derived(float zoom_units, float view_start, AutoConf *auto_conf, int cursor_x, int cursor_y, int shift_down) { return 0; };
	int release_auto();
	virtual int release_auto_derived() { return 0; };
// Return whether automation would prevent direct frame copies.  Not fully implemented.
	int automation_is_used(int64_t start, int64_t end);
	virtual int automation_is_used_derived(int64_t start, int64_t end) { return 0; }

	int popup_transition(int cursor_x, int cursor_y);

// Return 1 if the left handle was selected 2 if the right handle was selected 3 if the track isn't recordable
	int modify_edithandles(double oldposition, 
		double newposition, 
		int currentend, 
		int handle_mode,
		int edit_labels,
		int edit_plugins);
	int modify_pluginhandles(double oldposition, 
		double newposition, 
		int currentend, 
		int handle_mode,
		int edit_labels,
		Edits *trim_edits);
	int select_edit(int cursor_x, 
		int cursor_y, 
		double &new_start, 
		double &new_end);
	virtual int end_translation() { return 0; };
	virtual int reset_translation(int64_t start, int64_t end) { return 0; };
	int feather_edits(int64_t start, int64_t end, int64_t units);
	int64_t get_feather(int64_t selectionstart, int64_t selectionend);


// Absolute number of this track
	int number_of();           

// get_dimensions is used for getting drawing regions so use floats for partial frames
// get the display dimensions in SAMPLES OR FRAMES
	virtual int get_dimensions(double &view_start, 
		double &view_units, 
		double &zoom_units) { return 0; };   
// Longest time from current_position in which nothing changes
	int64_t edit_change_duration(int64_t input_position, 
		int64_t input_length, 
		int reverse, 
		int test_transitions,
		int use_nudge);
	int64_t plugin_change_duration(int64_t input_position,
		int64_t input_length,
		int reverse,
		int use_nudge);
// Utility for edit_change_duration.
	int need_edit(Edit *current, int test_transitions);
// If the edit under position is playable.
// Used by PlayableTracks::is_playable.
	int playable_edit(int64_t position, int direction);

// ===================================== for handles, titles, etc

	int64_t old_view_start;
	int pixel;   // pixel position from top of track view
// Dimensions of this track if video
	int track_w, track_h;




private:
// Identification of the track
	int id;
};

#endif

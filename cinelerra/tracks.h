#ifndef TRACKS_H
#define TRACKS_H


#include "autoconf.h"
#include "cursor.inc"
#include "edl.inc"
#include "file.inc"
#include "filexml.inc"
#include "linklist.h"
#include "pluginserver.inc"
#include "threadindexer.inc"
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
	int load(FileXML *xml, int &track_offset, uint32_t load_flags);
	void move_edits(ArrayList<Edit*> *edits, 
		Track *track,
		double position,
		int edit_labels,
		int edit_plugins);
	void move_effect(Plugin *plugin,
		PluginSet *plugin_set,
		Track *track, 
		int64_t position);

// Construct a list of all the recordable edits which start on position
	void get_affected_edits(ArrayList<Edit*> *drag_edits, 
		double position, 
		Track *start_track);

	void equivalent_output(Tracks *tracks, double *result);

	int move_track_up(Track *track);        // move recordable tracks up
	int move_track_down(Track *track);      // move recordable tracks down
	int move_tracks_up();                   // move recordable tracks up
	int move_tracks_down();                 // move recordable tracks down
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
// return the longest track in all the tracks in seconds
 	double total_length();
 	double total_video_length();
// Update y pixels after a zoom
	void update_y_pixels(Theme *theme);
// Total number of tracks where the following toggles are selected
	void select_all(int type,
		int value);
	void translate_camera(float offset_x, float offset_y);
	void translate_projector(float offset_x, float offset_y);
		int total_of(int type);
// add a track
	Track* add_audio_track(int above, Track *dst_track);
	Track* add_video_track(int above, Track *dst_track);
//	Track* add_audio_track(int to_end = 1);
//	Track* add_video_track(int to_end = 1);
	int delete_track();     // delete last track
	int delete_track(Track* track);        // delete any track

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



	
	
	
	
	
	
	int change_channels(int oldchannels, int newchannels);
	int dump();



// Change references to shared modules in all tracks from old to new.
// If do_swap is true values of new are replaced with old.
	void change_modules(int old_location, int new_location, int do_swap);
// Append all the tracks to the end of the recordable tracks
	int concatenate_tracks(int edit_plugins);
// Change references to shared plugins in all tracks
	void change_plugins(SharedLocation &old_location, SharedLocation &new_location, int do_swap);

	int delete_audio_track();       // delete the last audio track
	int delete_video_track();        // delete the last video track
	int delete_tracks();     // delete all the recordable tracks
	int delete_all_tracks();      // delete just the tracks

// ================================== EDL editing
	int copy(double start, 
		double end, 
		int all, 
		FileXML *file, 
		char *output_path = "");



	int copy_assets(FileXML *xml, 
		double start, 
		double end, 
		int all);
	int clear(double start, double end, int clear_plugins);
// Returns the point to restart background rendering at.
// -1 means nothing changed.
	void clear_automation(double selectionstart, 
		double selectionend);
	int clear_default_keyframe();
	int clear_handle(double start, 
		double end,
		double &longest_distance,
		int clear_labels,
		int clear_plugins);
	int copy_automation(double selectionstart, 
		double selectionend, 
		FileXML *file,
		int default_only,
		int autos_only);
	int copy_default_keyframe(FileXML *file);
	void paste_automation(double selectionstart, 
		FileXML *xml,
		int default_only);
	int paste_default_keyframe(FileXML *file);
	int paste(int64_t start, int64_t end);
// all units are samples by default
	int paste_output(int64_t startproject, 
				int64_t endproject, 
				int64_t startsource_sample, 
				int64_t endsource_sample, 
				int64_t startsource_frame, 
				int64_t endsource_frame, 
				Asset *asset);
	int paste_silence(double start, 
		double end, 
		int edit_plugins);
	int purge_asset(Asset *asset);
	int asset_used(Asset *asset);
// Transition popup
	int popup_transition(int cursor_x, int cursor_y);
	int select_auto(int cursor_x, int cursor_y);
	int move_auto(int cursor_x, int cursor_y, int shift_down);
	int modify_edithandles(double &oldposition, 
		double &newposition, 
		int currentend, 
		int handle_mode,
		int edit_labels,
		int edit_plugins);
	int modify_pluginhandles(double &oldposition, 
		double &newposition, 
		int currentend, 
		int handle_mode,
		int edit_labels);
	int select_handles();
	int select_region();
	int select_edit(int64_t cursor_position, int cursor_x, int cursor_y, int64_t &new_start, int64_t &new_end);
	int feather_edits(int64_t start, int64_t end, int64_t samples, int audio, int video);
	int64_t get_feather(int64_t selectionstart, int64_t selectionend, int audio, int video);
// Move edit boundaries and automation during a framerate change
	int scale_time(float rate_scale, int ignore_record, int scale_edits, int scale_autos, int64_t start, int64_t end);

// ================================== accounting

	int handles, titles;     // show handles or titles
	int show_output;         // what type of video to draw
	AutoConf auto_conf;      // which autos are visible
	int overlays_visible;
	double total_playable_length();     // Longest track.
// Used by equivalent_output
	int total_playable_vtracks();
	double total_recordable_length();   // Longest track with recording on
	int totalpixels();       // height of all tracks in pixels
	int number_of(Track *track);        // track number of pointer
	Track* number(int number);      // pointer to track number


private:
};

#endif

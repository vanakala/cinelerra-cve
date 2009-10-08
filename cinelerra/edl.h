
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

#ifndef EDL_H
#define EDL_H

#include "asset.inc"
#include "assets.inc"
#include "autoconf.inc"
#include "bchash.inc"
#include "edits.inc"
#include "edl.inc"
#include "edlsession.inc"
#include "filexml.inc"
#include "labels.inc"
#include "localsession.inc"
#include "maxchannels.h"
#include "mutex.inc"
#include "playbackconfig.h"
#include "pluginserver.h"
#include "preferences.inc"
#include "recordlabel.inc"
#include "sharedlocation.inc"
#include "theme.inc"
#include "tracks.inc"


// Loading and saving are built on load and copy except for automation:

// Storage: 
// Load: load new -> paste into master
// Save: copy all of master
// Undo: selective load into master
// Copy: copy from master
// Paste: load new -> paste into master
// Copy automation: copy just automation from master
// Paste automation: paste functions in automation











class EDL
{
public:
	EDL(EDL *parent_edl = 0);
	~EDL();

	int create_objects();
	EDL& operator=(EDL &edl);

// Load configuration and track counts
	int load_defaults(BC_Hash *defaults);
	int save_defaults(BC_Hash *defaults);
// Clip default settings to boundaries.
	void boundaries();
// Create tracks using existing configuration
	int create_default_tracks();
	int load_xml(ArrayList<PluginServer*> *plugindb, 
		FileXML *file, 
		uint32_t load_flags);
	int save_xml(ArrayList<PluginServer*> *plugindb,
		FileXML *xml, 
		char *output_path,
		int is_clip,
		int is_vwindow);
    int load_audio_config(FileXML *file, int append_mode, uint32_t load_flags);
    int load_video_config(FileXML *file, int append_mode, uint32_t load_flags);



// Convert position to frames if cursor alignment is enabled
	double align_to_frame(double position, int round);



// Scale all sample values since everything is locked to audio
	void rechannel();
	void resample(double old_rate, double new_rate, int data_type);
	void copy_tracks(EDL *edl);
// Copies project path, folders, EDLSession, and LocalSession from edl argument.
// session_only - used by preferences and format specify 
// whether to only copy EDLSession
	void copy_session(EDL *edl, int session_only = 0);
	int copy_all(EDL *edl);
	void copy_assets(EDL *edl);
	void copy_clips(EDL *edl);
// Copy pan and fade settings from edl
	void synchronize_params(EDL *edl);
// Determine if the positions are equivalent if they're within half a frame
// of each other.
	int equivalent(double position1, double position2);
// Determine if the EDL's produce equivalent video output to the old EDL.
// The new EDL is this and the old EDL is the argument.
// Return the number of seconds from the beginning of this which are 
// equivalent to the argument.
// If they're completely equivalent, -1 is returned;
// This is used by BRender.
	double equivalent_output(EDL *edl);
// Set project path for saving a backup
	void set_project_path(char *path);
// Set points and labels
	void set_inpoint(double position);
	void set_outpoint(double position);
// Redraw resources during index builds
	void set_index_file(Asset *asset);
// Add assets from the src to the destination
	void update_assets(EDL *src);
	void optimize();
// Debug
	int dump();
	static int next_id();
// Create a new folder if it doesn't exist already
	void new_folder(char *folder);
	void delete_folder(char *folder);
	void modify_edithandles(double oldposition, 
		double newposition, 
		int currentend, 
		int handle_mode,
		int edit_labels,
		int edit_plugins);

	void modify_pluginhandles(double oldposition, 
		double newposition, 
		int currentend, 
		int handle_mode,
		int edit_labels,
		Edits *trim_edits);

	int trim_selection(double start, 
		double end,
		int edit_labels,
		int edit_plugins);

// Editing functions
	int copy_assets(double start, 
		double end, 
		FileXML *file, 
		int all, 
		ArrayList<PluginServer*> *plugindb,
		char *output_path);
	int copy(double start, 
		double end, 
		int all,   // Ignore recordable status of tracks for saving
		int is_clip,
		int is_vwindow,
		FileXML *file, 
		ArrayList<PluginServer*> *plugindb,
		char *output_path,
		int rewind_it);     // Rewind EDL for easy pasting
	void paste_silence(double start, 
		double end, 
		int edit_labels /* = 1 */, 
		int edit_plugins);
	void remove_from_project(ArrayList<Asset*> *assets);
	void remove_from_project(ArrayList<EDL*> *clips);
	int clear(double start, 
		double end, 
		int clear_labels,
		int clear_plugins);
// Insert the asset at a point in the EDL
	void insert_asset(Asset *asset, 
		double position, 
		Track *first_track = 0, 
		RecordLabels *labels = 0);
// Insert the clip at a point in the EDL
	int insert_clips(ArrayList<EDL*> *new_edls, int load_mode, Track *first_track = 0);
// Add a copy of EDL* to the clip array.  Returns the copy.
	EDL* add_clip(EDL *edl);

	void get_shared_plugins(Track *source, ArrayList<SharedLocation*> *plugin_locations);
	void get_shared_tracks(Track *track, ArrayList<SharedLocation*> *module_locations);


    int get_tracks_height(Theme *theme);
    int64_t get_tracks_width();
// Return the dimension for a single pane if single_channel is set.
// Otherwise add all panes.
/*
 * 	int calculate_output_w(int single_channel);
 * 	int calculate_output_h(int single_channel);
 */
// Return dimensions for canvas if smaller dimensions has zoom of 1
	void calculate_conformed_dimensions(int single_channel, float &w, float &h);
// Get the total output size scaled to aspect ratio
	void output_dimensions_scaled(int &w, int &h);
	float get_aspect_ratio();

// Titles of all subfolders
	ArrayList<char*> folders;
// Clips
	ArrayList<EDL*> clips;
// VWindow
	EDL *vwindow_edl;
// is the vwindow_edl shared and therefore should not be deleted in destructor
	int vwindow_edl_shared;

// Media files
// Shared between all EDLs
	Assets *assets;



	Tracks *tracks;
	Labels *labels;
// Shared between all EDLs in a tree, for projects.
	EDLSession *session;
// Specific to this EDL, for clips.
	LocalSession *local_session;

// In the top EDL, this is the path it was loaded from.  Restores 
// project titles from backups.  This is only used for loading backups.
// All other loads keep the path in mainsession->filename.
// This can't use the output_path argument to save_xml because that points
// to the backup file, not the project file.
	char project_path[BCTEXTLEN];



// Use parent Assets if nonzero
	EDL *parent_edl;


	static Mutex *id_lock;

// unique ID of this EDL for resource window
	int id;
};

#endif

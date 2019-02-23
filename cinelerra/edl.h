
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
#include "autoconf.inc"
#include "bchash.inc"
#include "edits.inc"
#include "edl.inc"
#include "edlsession.inc"
#include "filexml.inc"
#include "labels.inc"
#include "localsession.inc"
#include "mutex.inc"
#include "playbackconfig.h"
#include "pluginserver.h"
#include "preferences.inc"
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

extern EDL *master_edl;
extern EDL *vwindow_edl;
extern EDL *render_edl;

class EDL
{
public:
	EDL(int is_master);
	~EDL();

	void reset_instance();
	EDL& operator=(EDL &edl);

// Load configuration and track counts
	void load_defaults(BC_Hash *defaults, EDLSession *session);
	void save_defaults(BC_Hash *defaults, EDLSession *session);
// Clip default settings to boundaries.
	void boundaries();
// Create tracks using existing configuration
	void create_default_tracks();
	void load_xml(FileXML *file, uint32_t load_flags, EDLSession *session);
	void save_xml(FileXML *xml, const char *output_path,
		int is_clip, int is_vwindow);
	int load_audio_config(FileXML *file, int append_mode, uint32_t load_flags);
	int load_video_config(FileXML *file, int append_mode, uint32_t load_flags);

// Convert position to frames if cursor alignment is enabled
	ptstime align_to_frame(ptstime position, int round = 1);

// Scale all sample values since everything is locked to audio
	void rechannel();
	void copy_tracks(EDL *edl);

// Copies project path, folders, and LocalSession from edl argument.
// Sets this_edlsession to session if given
	void copy_session(EDL *edl, EDLSession *session = 0);
	void copy_all(EDL *edl);

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
	void set_project_path(const char *path);

// Set points and labels
	void set_inpoint(ptstime position);
	void set_outpoint(ptstime position);

// Add assets from the src to the destination
	void update_assets(EDL *src);

	void update_assets(Asset *asset);
	void optimize();

// Debug
	void dump(int indent = 0);
	void dump_assets(int indent = 0);

	static int next_id();

	ptstime adjust_position(ptstime oldposition,
		ptstime newposition,
		int currentend,
		int handle_mode);

	void modify_edithandles(ptstime oldposition,
		ptstime newposition,
		int currentend, 
		int handle_mode,
		int actions);

	void modify_pluginhandles(ptstime oldposition,
		ptstime newposition,
		int currentend, 
		int handle_mode,
		int edit_labels);

	void trim_selection(ptstime start, 
		ptstime end,
		int actions);

// Editing functions
	void copy_assets(ptstime start,
		ptstime end,
		FileXML *file, 
		int all, 
		const char *output_path);
	void copy(EDL *edl, ptstime start, ptstime end,
		ArrayList<Track*> *src_tracks = 0);
	void paste_silence(ptstime start,
		ptstime end,
		int action);
	void remove_from_project(ArrayList<Asset*> *assets);
	void clear(ptstime start,
		ptstime end,
		int actions);
// Insert the asset at a point in the EDL
	void insert_asset(Asset *asset, 
		double position, 
		Track *first_track = 0);
// Insert the clip at a point in the EDL
	int insert_clips(ArrayList<EDL*> *new_edls, int load_mode, Track *first_track = 0);

	void get_shared_plugins(Track *source, ArrayList<SharedLocation*> *plugin_locations);
	void get_shared_tracks(Track *track, ArrayList<SharedLocation*> *module_locations);

	int get_tracks_height(Theme *theme);

// Return dimensions for canvas if smaller dimensions has zoom of 1
	void calculate_conformed_dimensions(double &w, double &h);
// Get the total output size scaled to aspect ratio
	void output_dimensions_scaled(int &w, int &h);
	double get_sample_aspect_ratio();
// Create tracks from assets
	void finalize_edl(int load_mode);

	ptstime total_length();
	ptstime total_length_of(int type);
	ptstime total_length_framealigned();
	int total_tracks();
	int total_tracks_of(int type);
	int playable_tracks_of(int type);
	int recordable_tracks_of(int type);
// Pointer to the first track
	Track *first_track();
// Number of pointed track
	int number_of(Track *track);
// Pointer to track number
	Track *number(int number);
// Set toggles of all tracks to value
	void set_all_toggles(int toggle_type, int value);
// Count number of toggels set
	int total_toggled(int toggle_type);
// Ensure that only one track is master
	void check_master_track();
// Name of the handle
	static const char *handle_name(int handle);

// Media files
// Shared between all EDLs
	ArrayList<Asset*> *assets;

	Tracks *tracks;
	Labels *labels;

// Specific to this EDL, for clips.
	LocalSession *local_session;
// EDLSession specific of this edl
// Not owned by this edl
	EDLSession *this_edlsession;

// In the top EDL, this is the path it was loaded from.  Restores 
// project titles from backups.  This is only used for loading backups.
// All other loads keep the path in mainsession->filename.
// This can't use the output_path argument to save_xml because that points
// to the backup file, not the project file.
	char project_path[BCTEXTLEN];

	static Mutex *id_lock;

// unique ID of this EDL for resource window
	int id;
	int is_master;
};

#endif

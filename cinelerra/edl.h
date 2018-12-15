
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

class EDL
{
public:
	EDL(EDL *parent_edl = 0);
	~EDL();

	EDL& operator=(EDL &edl);

// Load configuration and track counts
	void load_defaults(BC_Hash *defaults);
	void save_defaults(BC_Hash *defaults);
// Clip default settings to boundaries.
	void boundaries();
// Create tracks using existing configuration
	void create_default_tracks();
	void load_xml(FileXML *file, uint32_t load_flags);
	void save_xml(FileXML *xml, const char *output_path,
		int is_clip, int is_vwindow);
	int load_audio_config(FileXML *file, int append_mode, uint32_t load_flags);
	int load_video_config(FileXML *file, int append_mode, uint32_t load_flags);

// Convert position to frames if cursor alignment is enabled
	ptstime align_to_frame(ptstime position, int round = 1);

// Scale all sample values since everything is locked to audio
	void rechannel();
	void copy_tracks(EDL *edl);

// Copies project path, folders, EDLSession, and LocalSession from edl argument.
// session_only - used by preferences and format specify 
// whether to only copy EDLSession
	void copy_session(EDL *edl, int session_only = 0);
	void copy_all(EDL *edl);
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
	void copy(ptstime start,
		ptstime end,
		int all,   // Ignore recordable status of tracks for saving
		int is_clip,
		int is_vwindow,
		FileXML *file, 
		const char *output_path,
		int rewind_it);     // Rewind EDL for easy pasting
	void paste_silence(ptstime start,
		ptstime end,
		int edit_labels, 
		int edit_plugins);
	void remove_from_project(ArrayList<Asset*> *assets);
	void remove_from_project(ArrayList<EDL*> *clips);
	void clear(ptstime start,
		ptstime end,
		int actions);
// Insert the asset at a point in the EDL
	void insert_asset(Asset *asset, 
		double position, 
		Track *first_track = 0);
// Insert the clip at a point in the EDL
	int insert_clips(ArrayList<EDL*> *new_edls, int load_mode, Track *first_track = 0);
// Add a copy of EDL* to the clip array.  Returns the copy.
	EDL* add_clip(EDL *edl);

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

// Clips
	ArrayList<EDL*> clips;

// Media files
// Shared between all EDLs
	ArrayList<Asset*> *assets;

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

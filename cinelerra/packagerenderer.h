
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

#ifndef PACKAGERENDERER_H
#define PACKAGERENDERER_H


#include "assets.inc"
#include "bcwindowbase.inc"
#include "cache.inc"
#include "edit.inc"
#include "edl.inc"
#include "file.inc"
#include "maxchannels.h"
#include "mwindow.inc"
#include "playabletracks.inc"
#include "playbackconfig.inc"
#include "pluginserver.inc"
#include "preferences.inc"
#include "renderengine.inc"
#include "track.inc"
#include "transportque.inc"
#include "vframe.inc"
#include "videodevice.inc"


#include <stdint.h>

class RenderPackage
{
public:
	RenderPackage();
	~RenderPackage();

// Path of output without remote prefix
	char path[BCTEXTLEN];

// Range not including preroll
	int64_t audio_start;
	int64_t audio_end;
	int64_t video_start;
	int64_t video_end;
	int done;
	int use_brender;
	int video_do;
	int audio_do;
};




// Used by Render and BRender to do packages.
class PackageRenderer
{
public:
	PackageRenderer();
	~PackageRenderer();

// Initialize stuff which is reused between packages
	int initialize(MWindow *mwindow,
		EDL *edl, 
		Preferences *preferences, 
		Asset *default_asset,
		ArrayList<PluginServer*> *plugindb);

// Aborts and returns 1 if an error is encountered.
	int render_package(RenderPackage *package);

	int direct_copy_possible(EDL *edl,
		int64_t current_position, 
		Track* playable_track,  // The one track which is playable
		Edit* &playable_edit, // The edit which is playing
		File *file);   // Output file
	int direct_frame_copy(EDL *edl, 
		int64_t &video_position, 
		File *file,
		int &result);

// Invoke behavior for master node
	virtual int get_master();
// Get result status from server
	virtual int get_result();
	virtual void set_result(int value);
	virtual void set_progress(int64_t total_samples);
// Used by background rendering to mark a frame as finished.
// If the GUI is locked for a long time this may abort, 
// assuming the server crashed.
	virtual int set_video_map(int64_t position, int value);
	virtual int progress_cancelled();

	void create_output();
	void create_engine();
	void do_audio();
	void do_video();
	void stop_engine();
	void stop_output();
	void close_output();


// Passed in from outside
	EDL *edl;
	Preferences *preferences;
	Asset *default_asset;
	ArrayList<PluginServer*> *plugindb;

// Created locally
	Asset *asset;
	double **audio_output;
	int64_t audio_position;
	int64_t audio_preroll;
	int64_t audio_read_length;
	File *file;
// This is 1 if an error is encountered.
	int result;
	VFrame ***video_output;
// A nonzero mwindow signals master render engine to the engine.
// A zero mwindow signals client or non interactive.
	MWindow *mwindow;
	double *audio_output_ptr[MAX_CHANNELS];
	CICache *audio_cache;
	CICache *video_cache;
	VFrame *compressed_output;
	AudioOutConfig *aconfig;
	VideoOutConfig *vconfig;
//	PlaybackConfig *playback_config;
	PlayableTracks *playable_tracks;
	RenderEngine *render_engine;
	RenderPackage *package;
	TransportCommand *command;
	int direct_frame_copying;
	VideoDevice *video_device;
	VFrame *video_output_ptr;
	int64_t video_preroll;
	int64_t video_position;
	int64_t video_read_length;
	int64_t video_write_length;
	int64_t video_write_position;
};




#endif

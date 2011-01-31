
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
#include "datatype.h"
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
	void dump();

// Path of output without remote prefix
	char path[BCTEXTLEN];

// Range not including preroll
	ptstime audio_start_pts;
	ptstime audio_end_pts;
	ptstime video_start_pts;
	ptstime video_end_pts;
	framenum count;
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
		ptstime current_postime,
		Track* playable_track,  // The one track which is playable
		Edit* &playable_edit, // The edit which is playing
		File *file);   // Output file
	int direct_frame_copy(EDL *edl, 
		ptstime &video_position, 
		File *file,
		int &result);

// Invoke behavior for master node
	virtual int get_master() { return 0; };
// Get result status from server
	virtual int get_result() { return 0; };
	virtual void set_result(int value) {};
	virtual void set_progress(ptstime duration) {};
// Used by background rendering to mark a frame as finished.
// If the GUI is locked for a long time this may abort, 
// assuming the server crashed.
	virtual void set_video_map(ptstime start, ptstime end) {};
	virtual int progress_cancelled() { return 0; };

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
	ptstime audio_pts;
	ptstime audio_preroll;
	int audio_read_length;
	File *file;
// This is 1 if an error is encountered.
	int result;
	VFrame ***video_output;
// A nonzero mwindow signals master render engine to the engine.
// A zero mwindow signals client or non interactive.
	MWindow *mwindow;
	CICache *audio_cache;
	CICache *video_cache;
	VFrame *compressed_output;
	AudioOutConfig *aconfig;
	VideoOutConfig *vconfig;
	PlayableTracks *playable_tracks;
	RenderEngine *render_engine;
	RenderPackage *package;
	TransportCommand *command;
	int direct_frame_copying;
	VideoDevice *video_device;
	VFrame *video_output_ptr;
	ptstime video_preroll;
	ptstime video_pts;
	ptstime video_read_length;
	ptstime brender_base;
	int video_write_length;
	int video_write_position;
};

#endif

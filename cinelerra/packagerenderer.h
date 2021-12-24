// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PACKAGERENDERER_H
#define PACKAGERENDERER_H

#include "aframe.inc"
#include "bcwindowbase.inc"
#include "cinelerra.h"
#include "edit.inc"
#include "datatype.h"
#include "edl.inc"
#include "file.inc"
#include "mwindow.inc"
#include "playbackconfig.inc"
#include "pluginserver.inc"
#include "preferences.inc"
#include "renderengine.inc"
#include "track.inc"
#include "transportcommand.inc"
#include "vframe.inc"
#include "videodevice.inc"

#include <stdint.h>

class RenderPackage
{
public:
	RenderPackage();

	void dump(int indent = 0);

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
	void initialize(EDL *edl,
		Preferences *preferences,
		Asset *default_asset);

// Aborts and returns 1 if an error is encountered.
	int render_package(RenderPackage *package);

// Invoke behavior for master node
	virtual int get_master() { return 0; };
// Get result status from server
	virtual int get_result() { return 0; };
	virtual void set_result(int value, const char *msg = 0) {};
	virtual void set_progress(ptstime duration) {};
// Used by background rendering to mark a frame as finished.
// If the GUI is locked for a long time this may abort, 
// assuming the server crashed.
	virtual void set_video_map(ptstime start, ptstime end) {};
	virtual int progress_cancelled() { return 0; };

	int create_output();
	void create_engine();
	int do_audio();
	int do_video();
	void stop_engine();
	void stop_output();
	void close_output();

// Passed in from outside
	EDL *edl;
	Preferences *preferences;
	Asset *default_asset;
	ptstime audio_pts;
	ptstime audio_preroll;
	int audio_read_length;
	File *file;
	AFrame *audio_output[MAX_CHANNELS];
	VFrame *video_output[MAX_CHANNELS];
// A nonzero mwindow signals master render engine to the engine.
// A zero mwindow signals client or non interactive.
	AudioOutConfig *aconfig;
	VideoOutConfig *vconfig;
	RenderEngine *render_engine;
	RenderPackage *package;
	TransportCommand *command;
	VideoDevice *video_device;
	ptstime video_preroll;
	ptstime video_pts;
	ptstime video_read_length;
	ptstime brender_base;
	char pkg_error[BCTEXTLEN];
};

#endif

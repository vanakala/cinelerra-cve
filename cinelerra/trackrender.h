// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef TRACKRENDER_H
#define TRACKRENDER_H

#include "aframe.inc"
#include "asset.inc"
#include "edit.inc"
#include "file.inc"
#include "plugin.inc"
#include "pluginclient.inc"
#include "track.inc"
#include "trackrender.inc"
#include "vframe.inc"

#define TRACKRENDER_FILES_MAX 4

class TrackRender
{
	friend class AudioRender;
	friend class VideoRender;
public:
	TrackRender(Track *track);
	virtual ~TrackRender();

	virtual VFrame *get_vframe(VFrame *buffer) { return 0; };
	virtual VFrame *get_vtmpframe(VFrame *buffer,
		PluginClient *client) { return 0; };
	virtual AFrame *get_aframe(AFrame *buffer) { return 0; };
	virtual AFrame *get_atmpframe(AFrame *buffer,
		PluginClient *client) { return 0; };
	virtual void copy_track_aframe(AFrame *buffer) {};
	virtual VFrame *copy_track_vframe(VFrame *buffer) {};
	Track *get_track_number(int number);
	void set_effects_track(Track *track);
	int track_ready();
	int is_muted(ptstime pts);
	void release_asset(Asset *asset);

	Track *media_track;
	Plugin *next_plugin;
	int initialized_buffers;

protected:
	File *media_file(Edit *edit, int filenum);
	int is_playable(ptstime pts, Edit *edit);
	ptstime align_to_frame(ptstime position);
	void dump(int indent);

	Track *plugins_track;
	Track *autos_track;

private:
	File *trackfiles[TRACKRENDER_FILES_MAX];
};

#endif

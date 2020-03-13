
/*
 * CINELERRA
 * Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>
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

#ifndef TRACKRENDER_H
#define TRACKRENDER_H

#include "aframe.inc"
#include "asset.inc"
#include "edit.inc"
#include "file.inc"
#include "plugin.inc"
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
	virtual AFrame *get_aframe(AFrame *buffer) { return 0; };
	virtual void copy_track_aframe(AFrame *buffer) {};
	virtual VFrame *copy_track_vframe(VFrame *buffer, int use_tmpframe) {};
	Track *get_track_number(int number);
	void set_effects_track(Track *track);
	int track_ready();
	int is_muted(ptstime pts);

	Track *media_track;
	Plugin *next_plugin;

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

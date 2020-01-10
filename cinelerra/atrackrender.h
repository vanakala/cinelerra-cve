
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

#ifndef ATRACKRENDER_H
#define ATRACKRENDER_H

#include "aframe.inc"
#include "atrackrender.inc"
#include "cinelerra.h"
#include "edit.inc"
#include "levelhist.h"
#include "plugin.inc"
#include "track.inc"
#include "audiorender.inc"
#include "trackrender.h"

class ATrackRender : public TrackRender
{
public:
	ATrackRender(Track *track, AudioRender *arender);
	~ATrackRender();

	void get_aframes(AFrame **output, int out_channels, int rstp);
	AFrame *get_aframe(AFrame *buffer);
	void render_pan(AFrame **output, int out_channels);
	void copy_track_aframe(AFrame *aframe);

	LevelHistory module_levels;
private:
	void render_fade(AFrame *aframe);
	void render_transition(AFrame *aframe, Edit *edit);
	void render_plugins(AFrame *aframe, Edit *edit, int rstep);
	AFrame *execute_plugin(Plugin *plugin, AFrame *aframe, int rstep);

	AudioRender *arender;
	Edit *current_edit;
	AFrame *track_frame;
};

#endif

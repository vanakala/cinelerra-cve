
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

#ifndef VTRACKRENDER_H
#define VTRACKRENDER_H

#include "cropengine.inc"
#include "fadeengine.inc"
#include "maskengine.inc"
#include "track.inc"
#include "trackrender.h"
#include "vframe.inc"
#include "vtrackrender.inc"

class VTrackRender : public TrackRender
{
public:
	VTrackRender(Track *track);
	~VTrackRender();

	VFrame *get_frame(VFrame *frame);
private:
	void render_fade(VFrame *frame);
	void render_mask(VFrame *frame);
	void render_crop(VFrame *frame);

	FadeEngine *fader;
	MaskEngine *masker;
	CropEngine *cropper;
};

#endif

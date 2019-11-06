
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
#include "edit.inc"
#include "fadeengine.inc"
#include "maskengine.inc"
#include "overlayframe.inc"
#include "plugin.inc"
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
	VFrame *get_vframe(VFrame *buffer);
private:
	void render_fade(VFrame *frame);
	void render_mask(VFrame *frame, int before_plugins);
	void render_crop(VFrame *frame, int before_plugins);
	VFrame *render_projector(VFrame *frame);
	VFrame *render_camera(VFrame *frame);
	void calculate_input_transfer(ptstime position,
		int *in_x1, int *in_y1, int *in_x2, int *in_y2,
		int *out_x1, int *out_y1, int *out_x2, int *out_y2);
	void calculate_output_transfer(VFrame *output,
		int *in_x1, int *in_y1, int *in_x2, int *in_y2,
		int *out_x1, int *out_y1, int *out_x2, int *out_y2);
	VFrame *render_plugins(VFrame *frame, Edit *edit);
	VFrame *execute_plugin(Plugin *plugin, VFrame *frame);

	FadeEngine *fader;
	MaskEngine *masker;
	CropEngine *cropper;
	OverlayFrame *overlayer;
	Edit *current_edit;
};

#endif

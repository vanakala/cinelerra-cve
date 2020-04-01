
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

#include "arraylist.h"
#include "cropengine.inc"
#include "edit.inc"
#include "fadeengine.inc"
#include "maskengine.inc"
#include "overlayframe.inc"
#include "plugin.inc"
#include "pluginclient.inc"
#include "track.inc"
#include "videorender.inc"
#include "trackrender.h"
#include "vframe.inc"
#include "vtrackrender.inc"

class VTrackRender : public TrackRender
{
public:
	VTrackRender(Track *track, VideoRender *vrender);
	~VTrackRender();

	void process_vframe(ptstime pts, int rstep);
	VFrame *get_vframe(VFrame *buffer);
	VFrame *get_vtmpframe(VFrame *buffer, PluginClient *client);
	VFrame *handover_trackframe();
	void take_vframe(VFrame *frame);
	VFrame *copy_track_vframe(VFrame *vframe);
	VFrame *render_projector(VFrame *output, VFrame **input = 0);
	void dump(int indent);

// Frames for multichannel plugin
	ArrayList<VFrame*> vframes;
	int initialized_buffers;

private:
	void read_vframe(VFrame *vframe, Edit *edit, int filenum = 0);
	void render_fade(VFrame *frame);
	void render_mask(VFrame *frame, int before_plugins);
	void render_crop(VFrame *frame, int before_plugins);
	VFrame *render_camera(VFrame *frame);
	void calculate_input_transfer(ptstime position,
		int *in_x1, int *in_y1, int *in_x2, int *in_y2,
		int *out_x1, int *out_y1, int *out_x2, int *out_y2);
	void calculate_output_transfer(VFrame *output,
		int *in_x1, int *in_y1, int *in_x2, int *in_y2,
		int *out_x1, int *out_y1, int *out_x2, int *out_y2);
	VFrame *render_plugins(VFrame *frame, Edit *edit, int rstep);
	VFrame *execute_plugin(Plugin *plugin, VFrame *frame, int rstep);
	VFrame *render_transition(VFrame *frame, Edit *edit);
	int need_camera(ptstime pts);

	VideoRender *videorender;
	FadeEngine *fader;
	MaskEngine *masker;
	CropEngine *cropper;
	OverlayFrame *overlayer;
	Edit *current_edit;
	VFrame *track_frame;
	VFrame *plugin_frame;
};

#endif

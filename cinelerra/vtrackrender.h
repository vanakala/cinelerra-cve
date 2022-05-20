// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>


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
	VFrame *get_vtmpframe(VFrame *buffer, PluginClient *client);
	VFrame *handover_trackframe();
	void take_vframe(VFrame *frame);
	VFrame *render_projector(VFrame *output, VFrame **input = 0);
	void calculate_output_transfer(VFrame *output,
		int *in_x1, int *in_y1, int *in_x2, int *in_y2,
		int *out_x1, int *out_y1, int *out_x2, int *out_y2);
	void dump(int indent);

// Frames for multichannel plugin
	ArrayList<VFrame*> vframes;

private:
	void read_vframe(VFrame *vframe, Edit *edit, int filenum = 0);
	void render_fade(VFrame *frame);
	void render_mask(VFrame *frame, int before_plugins);
	void render_crop(VFrame *frame, int before_plugins);
	VFrame *render_camera(VFrame *frame);
	void calculate_input_transfer(ptstime position,
		int *in_x1, int *in_y1, int *in_x2, int *in_y2,
		int *out_x1, int *out_y1, int *out_x2, int *out_y2);
	VFrame *render_plugins(VFrame *frame, Edit *edit);
	VFrame *execute_plugin(Plugin *plugin, VFrame *frame, Edit *edit);
	VFrame *render_transition(VFrame *frame, Edit *edit);
	int need_camera(ptstime pts);

	VideoRender *videorender;
	FadeEngine *fader;
	MaskEngine *masker;
	CropEngine *cropper;
	OverlayFrame *overlayer;
	VFrame *track_frame;
};

#endif

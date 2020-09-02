// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>


#ifndef ATRACKRENDER_H
#define ATRACKRENDER_H

#include "aframe.inc"
#include "atrackrender.inc"
#include "cinelerra.h"
#include "edit.inc"
#include "levelhist.h"
#include "plugin.inc"
#include "pluginclient.inc"
#include "track.inc"
#include "audiorender.inc"
#include "trackrender.h"

class ATrackRender : public TrackRender
{
public:
	ATrackRender(Track *track, AudioRender *arender);
	~ATrackRender();

	void process_aframes(AFrame **output, int out_channels, int rstp);
	AFrame *get_atmpframe(AFrame *buffer, PluginClient *client);
	void render_pan(AFrame **output, int out_channels);
	AFrame *handover_trackframe();
	void take_aframe(AFrame *frame);
	void dump(int indent);

	LevelHistory module_levels;

// Frames for multichannel plugin
	ArrayList<AFrame*> aframes;

private:
	void render_fade(AFrame *aframe);
	void render_transition(AFrame *aframe, Edit *edit);
	AFrame *render_plugins(AFrame *aframe, Edit *edit, int rstep);
	AFrame *execute_plugin(Plugin *plugin, AFrame *aframe, Edit *edit, int rstep);

	AudioRender *arender;
	AFrame *track_frame;
};

#endif

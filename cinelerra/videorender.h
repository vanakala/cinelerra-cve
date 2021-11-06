// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef VIDEORENDER_H
#define VIDEORENDER_H

#include "arraylist.h"
#include "asset.inc"
#include "bctimer.h"
#include "edl.inc"
#include "file.inc"
#include "renderbase.h"
#include "renderengine.inc"
#include "videorender.inc"
#include "vframe.inc"
#include "vtrackrender.inc"

class VideoRender : public RenderBase
{
public:
	VideoRender(RenderEngine *renderengine, EDL *edl);
	~VideoRender();

	void run();
	VFrame *process_buffer(VFrame *buffer);
	void pass_vframes(Plugin *plugin, VFrame *current_frame,
		VTrackRender *current_renderer);
	VFrame *take_vframes(Plugin *plugin, VTrackRender *current_renderer);
	void release_asset(Asset *asset);
	void reset_brender();

private:
	void process_frame(ptstime pts);
	void flash_output();
	void get_frame(ptstime pts);

// Statistics
	int frame_count;
	uint64_t sum_delay;
	int late_frame;
	int framerate_counter;
	Timer framerate_timer;

	ptstime flashed_pts;
	ptstime flashed_duration;

	File *brender_file;
	VFrame *frame;

};

#endif

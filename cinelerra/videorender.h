
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

#ifndef VIDEORENDER_H
#define VIDEORENDER_H

#include "bctimer.h"
#include "edl.inc"
#include "file.inc"
#include "renderbase.h"
#include "renderengine.inc"
#include "bctimer.h"
#include "videorender.inc"
#include "vframe.inc"

class VideoRender : public RenderBase
{
public:
	VideoRender(RenderEngine *renderengine, EDL *edl);
	~VideoRender();

	void run();
	VFrame *process_buffer(VFrame *buffer);

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

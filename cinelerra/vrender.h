
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#ifndef VRENDER_H
#define VRENDER_H

#include "bctimer.h"
#include "commonrender.h"
#include "edit.inc"
#include "mwindow.inc"
#include "overlayframe.inc"
#include "renderengine.inc"
#include "vframe.inc"


class VRender : public CommonRender
{
public:
	VRender(RenderEngine *renderengine);
	VRender(MWindow *mwindow, RenderEngine *renderengine);
	~VRender();

	VirtualConsole* new_vconsole_object();
	int get_total_tracks();
	Module* new_module(Track *track);

	void run();

	int start_playback();     // start the thread

// get data type for certain commonrender routines
	int get_datatype();

// process frames to put in buffer_out
	void process_buffer(VFrame *video_out);

// load an array of buffers for each track to send to the thread
	void process_buffer(ptstime input_postime);

// Flash the output on the display
	int flash_output();

// Engine for camera and projector automation
	OverlayFrame *overlayer;

// Flag first frame to unlock audio
	int first_frame;

private:
// Output frame
	VFrame *video_out;

// for getting actual framerate
	int framerate_counter;
	Timer framerate_timer;

// Last frame displayed
	ptstime flashed_pts;
};

#endif

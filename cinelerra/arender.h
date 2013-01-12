
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

#ifndef ARENDER_H
#define ARENDER_H

#include "atrack.inc"
#include "aframe.inc"
#include "commonrender.h"
#include "levelhist.inc"
#include "maxchannels.h"

class ARender : public CommonRender
{
public:
	ARender(RenderEngine *renderengine);
	~ARender();

	void arm_command();
	void init_output_buffers();
	VirtualConsole* new_vconsole_object();
	int get_total_tracks();
	Module* new_module(Track *track);

	posnum tounits(ptstime position, int round = 0);
	ptstime fromunits(posnum position);

	void run();

// output buffers for audio device
	AFrame *audio_out[MAXCHANNELS];

// Make VirtualAConsole block before the first buffer until video is ready
	int first_buffer;


// get the data type for certain commonrender routines
	int get_datatype();

// process a buffer
// renders into buffer_out argument when no audio device
// handles playback autos
	int process_buffer(AFrame **buffer_out);
// renders to a device when there's a device
	int process_buffer(int input_len, ptstime input_postime);

	void send_last_buffer();
	int wait_device_completion();

	LevelHistory *output_levels;
};
#endif

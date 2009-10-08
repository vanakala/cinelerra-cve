
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

#ifndef VRENDERTHREAD_H
#define VRENDERTHREAD_H

#include "guicast.h"
#include "maxbuffers.h"
#include "vframe.inc"
#include "videodevice.inc"
#include "virtualconsole.h"
#include "vrender.inc"
#include "vtrack.inc"

class VirtualVConsole : public VirtualConsole
{
public:
	VirtualVConsole(RenderEngine *renderengine, VRender *vrender);
	virtual ~VirtualVConsole();

// Create ptrs to input buffers
/*
 * 	void new_input_buffer(int ring_buffer);
 * 	void delete_input_buffer(int ring_buffer);
 * 	int total_ring_buffers();
 */
	void get_playable_tracks();
	VirtualNode* new_entry_node(Track *track, 
		Module *module, 
		int track_number);

	VDeviceBase* get_vdriver();

// Composite a frame
// start_position - start of buffer in project if forward. end of buffer if reverse
	int process_buffer(int64_t input_position);

// absolute frame the buffer starts on
	int64_t absolute_frame;        

	VFrame *output_temp;
	VRender *vrender;
// Calculated at the start of every process_buffer
	int use_opengl;
};











#endif

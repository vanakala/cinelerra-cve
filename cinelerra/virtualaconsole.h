
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

#ifndef ARENDERTHREAD_H
#define ARENDERTHREAD_H


#include "arender.inc"
#include "filethread.inc"     // RING_BUFFERS
#include "virtualconsole.h"

class VirtualAConsole : public VirtualConsole
{
public:
	VirtualAConsole(RenderEngine *renderengine, ARender *arender);
	virtual ~VirtualAConsole();

	int set_transport(int reverse, float speed);
	void get_playable_tracks();

// process a buffer
	int process_buffer(int buffer, 
		int64_t input_len, 
		int64_t input_position, 
		int64_t absolute_position);

	int process_buffer(int64_t input_len,
		int64_t input_position,
		int last_buffer,
		int64_t absolute_position);

	void process_asynchronous();

// set up virtual console and buffers
	int init_rendering(int duplicate);
//	int build_virtual_console(int duplicate, int64_t current_position);
	VirtualNode* new_entry_node(Track *track, 
		Module *module,
		int track_number);

// cause audio device to quit
	int send_last_output_buffer();

// Temporary for audio rendering.  This stores each track's output before it is
// mixed into the device buffer.
	double *output_temp;
	int output_allocation;

	ARender *arender;
};


#endif

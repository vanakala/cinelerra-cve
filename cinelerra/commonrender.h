
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

#ifndef COMMONRENDER_H
#define COMMONRENDER_H

#include "cache.inc"
#include "condition.inc"
#include "datatype.h"
#include "virtualconsole.inc"
#include "module.inc"
#include "mwindow.inc"
#include "renderengine.inc"
#include "thread.h"
#include "track.inc"

#include <stdint.h>

class CommonRender : public Thread
{
public:
	CommonRender(RenderEngine *renderengine);
	virtual ~CommonRender();

	virtual void arm_command();
	virtual int get_total_tracks() { return 0; };
	virtual Module* new_module(Track *track) { return 0; };
	void delete_vconsole();
	void create_modules();
	void reset_parameters();
// Build the virtual console at the current position
	virtual void build_virtual_console();
	virtual VirtualConsole* new_vconsole_object() { return 0; };
	virtual void init_output_buffers() {};
	void start_plugins();
	void stop_plugins();
	int test_reconfigure(ptstime &length);

	void evaluate_current_position();
	void start_command();
	void restart_playback(void);
	virtual void run();

	RenderEngine *renderengine;
// Virtual console
	VirtualConsole *vconsole;
// Native units position in project used for all functions
	posnum current_position;
// Pts units position in project used for all functions
	ptstime current_postime;

	Condition *start_lock;
// flag for normally completed playback
	int done;
// Flag for interrupted playback
	int interrupt;
// flag for last buffer to be played back
	int last_playback;  
// if this media type is being rendered asynchronously by threads
	int asynchronous;
// Module for every track to dispatch plugins in whether the track is
// playable or not.
// Maintain module structures here instead of reusing the EDL so 
// plugins persist if the EDL is replaced.
// Modules can persist after vconsole is restarted.
	int total_modules;
	Module **modules;
	int data_type;
// If a VirtualConsole was created need to start plugins
	int restart_plugins;




	CommonRender(MWindow *mwindow, RenderEngine *renderengine);

// clean up rendering
	int virtual stop_rendering() {};
	int wait_for_completion();
	virtual int wait_device_completion() {};
// renders to a device when there's a device
	virtual int process_buffer(posnum input_len, posnum input_position) {};
	virtual int get_datatype() {};
// test region against loop boundaries
	int get_boundaries(posnum &current_render_length);
// advance the buffer position depending on the loop status
	void advance_position(posnum current_render_length);

// convert to and from the native units of the render engine
	virtual posnum tounits(ptstime position, int round);
	virtual ptstime fromunits(posnum position);

	MWindow *mwindow;

	int input_length;           // frames/samples to read from disk at a time
};

#endif

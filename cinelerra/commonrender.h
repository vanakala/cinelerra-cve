
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
	int test_reconfigure(int64_t position, int64_t &length);

	void evaluate_current_position();
	void start_command();
	virtual int restart_playback();
	virtual void run();

	RenderEngine *renderengine;
// Virtual console
	VirtualConsole *vconsole;
// Native units position in project used for all functions
	int64_t current_position;       
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
	virtual int process_buffer(int64_t input_len, int64_t input_position) {};

	virtual int get_datatype() {};
// test region against loop boundaries
	int get_boundaries(int64_t &current_render_length);
// test region for playback automation changes
	int get_automation(int64_t &current_render_length, int data_type);
// advance the buffer position depending on the loop status
	int advance_position(int64_t current_render_length);

// convert to and from the native units of the render engine
	virtual int64_t tounits(double position, int round);
	virtual double fromunits(int64_t position);
	virtual int64_t get_render_length(int64_t current_render_length) {};

	MWindow *mwindow;

	int64_t input_length;           // frames/samples to read from disk at a time

protected:
// make sure automation agrees with playable tracks
// automatically tests direction of playback
// return 1 if it doesn't
	int test_automation_before(int64_t &current_render_length, int data_type);
	int test_automation_after(int64_t &current_render_length, int data_type);
};


#endif

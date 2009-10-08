
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
#include "commonrender.h"
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

// set up and start thread
	int arm_playback(int64_t current_position,
				int64_t input_length, 
				int64_t module_render_fragment, 
				int64_t playback_buffer, 
				int64_t output_length);
	int wait_for_startup();
	int64_t tounits(double position, int round);
	double fromunits(int64_t position);

	void run();
// Calculate number of samples in each meter fragment and how many
// meter fragments to buffer.
	int calculate_history_size();
// Get subscript of history entry corresponding to sample
	int get_history_number(int64_t *table, int64_t position);

// output buffers for audio device
	double *audio_out[MAXCHANNELS];
// information for meters
	int get_next_peak(int current_peak);
// samples to use for one meter update.  Must be multiple of fragment_len
	int64_t meter_render_fragment;
// Level history of output buffers
	double *level_history[MAXCHANNELS];
// sample position of each level
	int64_t *level_samples;
// total entries in level_history
	int total_peaks;
// Next level to store value in
	int current_level[MAXCHANNELS];
// Make VirtualAConsole block before the first buffer until video is ready
	int first_buffer;







// get the data type for certain commonrender routines
	int get_datatype();

// handle playback autos and transitions
//	int restart_playback();
//	int send_reconfigure_buffer();

// process a buffer
// renders into buffer_out argument when no audio device
// handles playback autos
	int process_buffer(double **buffer_out, int64_t input_len, int64_t input_position, int last_buffer);
// renders to a device when there's a device
	int process_buffer(int64_t input_len, int64_t input_position);

	void send_last_buffer();
	int wait_device_completion();

// reverse the data in a buffer	
	int reverse_buffer(double *buffer, int64_t len);
// advance the buffer count
	int swap_current_buffer();
	int64_t get_render_length(int64_t current_render_length);

//	int64_t playback_buffer;            // maximum length to send to audio device
//	int64_t output_length;              // maximum length to send to audio device after speed


	int64_t source_length;  // Total number of frames to render for transitions


private:
// initialize buffer_out
	int init_meters();
// Samples since start of playback
	int64_t session_position;
};



#endif

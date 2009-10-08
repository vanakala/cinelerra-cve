
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

#ifndef PLUGINACLIENT_H
#define PLUGINACLIENT_H



#include "maxbuffers.h"
#include "pluginclient.h"

class PluginAClient : public PluginClient
{
public:
	PluginAClient(PluginServer *server);
	virtual ~PluginAClient();
	
	int get_render_ptrs();
	int init_realtime_parameters();

	int is_audio();
// These should return 1 if error or 0 if success.
// Multichannel buffer process for backwards compatibility
	virtual int process_realtime(int64_t size, 
		double **input_ptr, 
		double **output_ptr);
// Single channel buffer process for backwards compatibility and transitions
	virtual int process_realtime(int64_t size, 
		double *input_ptr, 
		double *output_ptr);

// Process buffer using pull method.  By default this loads the input into the
// buffer and calls process_realtime with input and output pointing to buffer.
// start_position - requested position relative to sample_rate. Relative
//     to start of EDL.  End of buffer if reverse.
// sample_rate - scale of start_position.
	virtual int process_buffer(int64_t size, 
		double **buffer,
		int64_t start_position,
		int sample_rate);
	virtual int process_buffer(int64_t size, 
		double *buffer,
		int64_t start_position,
		int sample_rate);


	virtual int process_loop(double *buffer, int64_t &write_length) { return 1; };
	virtual int process_loop(double **buffers, int64_t &write_length) { return 1; };
	int plugin_process_loop(double **buffers, int64_t &write_length);

	int plugin_start_loop(int64_t start, 
		int64_t end, 
		int64_t buffer_size, 
		int total_buffers);

	int plugin_get_parameters();

// Called by non-realtime client to read audio for processing.
// buffer - output wave
// channel - channel of the plugin input for multichannel plugin
// start_position - start of samples in forward.  End of samples in reverse.
//     Relative to start of EDL.  Scaled to sample_rate.
// len - number of samples to read
	int read_samples(double *buffer, 
		int channel, 
		int64_t start_position, 
		int64_t len);
	int read_samples(double *buffer, 
		int64_t start_position, 
		int64_t len);

// Called by realtime plugin to read audio from previous entity
// sample_rate - scale of start_position.  Provided so the client can get data
//     at a higher fidelity than provided by the EDL.
	int read_samples(double *buffer,
		int channel,
		int sample_rate,
		int64_t start_position,
		int64_t len);

// Get the sample rate of the EDL
	int get_project_samplerate();
// Get the requested sample rate
	int get_samplerate();

	int64_t local_to_edl(int64_t position);
	int64_t edl_to_local(int64_t position);

	void send_render_gui(void *data, int size);
	void plugin_render_gui(void *data, int size);
	virtual void render_gui(void *data, int size) {};

// point to the start of the buffers
	ArrayList<float**> input_ptr_master;
	ArrayList<float**> output_ptr_master;
// point to the regions for a single render
	float **input_ptr_render;
	float **output_ptr_render;
// sample rate of EDL.  Used for normalizing keyframes
	int project_sample_rate;
// Local parameters set by non realtime plugin about the file to be generated.
// Retrieved by server to set output file format.
// In realtime plugins, these are set before every process_buffer as the
// requested rates.
	int sample_rate;
};



#endif


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

#ifndef VIRTUALANODE_H
#define VIRTUALANODE_H


#include "arender.inc"
#include "filethread.inc"  // RING_BUFFERS
#include "maxchannels.h"
#include "plugin.inc"
#include "units.h"
#include "virtualnode.h"

class VirtualANode : public VirtualNode
{
public:
	VirtualANode(RenderEngine *renderengine, 
		VirtualConsole *vconsole, 
		Module *real_module, 
		Plugin *real_plugin,
		Track *track, 
		VirtualNode *parent_module);

	~VirtualANode();

	VirtualNode* create_module(Plugin *real_plugin, 
							Module *real_module, 
							Track *track);
	VirtualNode* create_plugin(Plugin *real_plugin);

// Called by VirtualAConsole::process_buffer to process exit_nodes.
// read_data recurses down the tree.
	int render(double *output_temp,
		int64_t start_position,
		int64_t len,
		int64_t sample_rate);

// Read data from whatever comes before this node.
// Calls render in either the parent node or the module for the track.
	int read_data(double *output_temp,
		int64_t start_position,
		int64_t len,
		int64_t sample_rate);

private:
// need *arender for peak updating
	int render_as_module(double **audio_out, 
					double *output_temp,
					int64_t start_position,
					int64_t len, 
					int64_t sample_rate);
	void render_as_plugin(double *output_temp,
		int64_t start_position, 
		int64_t len,
		int64_t sample_rate);

	int render_fade(double *buffer,
					int64_t len,
					int64_t input_position,
					int64_t sample_rate,
					Autos *autos,
					int direction,
					int use_nudge);
	int render_pan(double *input,        // start of input fragment
				double *output,        // start of output fragment
				int64_t fragment_len,      // fragment length in input scale
				int64_t input_position, // starting sample of input buffer in project
				int64_t sample_rate,
				Autos *autos,
				int channel,
				int direction,
				int use_nudge);
	void get_pan_automation(double &slope,
		double &intercept,
		int64_t input_position,
		int64_t &slope_len,
		Autos *autos,
		int channel,
		int direction);

/*
 * 	double* get_module_input(int double_buffer, int64_t fragment_position);
 * 	double* get_module_output(int double_buffer, int64_t fragment_position);
 */

	DB db;

	Auto *pan_before[MAXCHANNELS], *pan_after[MAXCHANNELS];
};


#endif

#ifndef VIRTUALANODE_H
#define VIRTUALANODE_H


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
		VirtualNode *parent_module, 
		double *buffer_in[],
		double *buffer_out[],
		int input_is_master,
		int output_is_master,
		int in,
		int out);

	~VirtualANode();

	void new_output_buffer();
	void new_input_buffer();
	VirtualNode* create_module(Plugin *real_plugin, 
							Module *real_module, 
							Track *track);
	VirtualNode* create_plugin(Plugin *real_plugin);

// need *arender for peak updating
	int render(double **audio_out, 
				long audio_out_position, 
				int double_buffer,
				long fragment_position,
				long fragment_len, 
				long real_position, 
				long source_length, 
				int reverse,
				ARender *arender);

// Pointers to data, whether drive read buffers or temp buffers
	double *buffer_in[RING_BUFFERS];
	double *buffer_out[RING_BUFFERS];

private:
// need *arender for peak updating
	int render_as_module(double **audio_out, 
				long audio_out_position, 
				int ring_buffer,
				long fragment_position,
				long fragment_len, 
				long real_position, 
				ARender *arender);
	void render_as_plugin(long real_position, 
		long fragment_position, 
		long fragment_len,
		int ring_buffer);

	int render_fade(double *input,        // start of input fragment
				double *output,        // start of output fragment
				long buffer_len,      // fragment length in input scale
				long input_position, // starting sample of input buffer in project
				Autos *autos);     // DB not used in pan
	int render_pan(double *input,        // start of input fragment
				double *output,        // start of output fragment
				long fragment_len,      // fragment length in input scale
				long input_position, // starting sample of input buffer in project
				Autos *autos,
				int channel);

	double* get_module_input(int double_buffer, long fragment_position);
	double* get_module_output(int double_buffer, long fragment_position);

	DB db;

	Auto *pan_before[MAXCHANNELS], *pan_after[MAXCHANNELS];
};


#endif

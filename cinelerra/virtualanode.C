#include "aattachmentpoint.h"
#include "amodule.h"
#include "arender.h"
#include "atrack.h"
#include "automation.h"
#include "edl.h"
#include "edlsession.h"
#include "floatautos.h"
#include "mwindow.h"
#include "module.h"
#include "plugin.h"
#include "renderengine.h"
#include "track.h"
#include "transition.h"
#include "transportque.h"
#include "virtualaconsole.h"
#include "virtualanode.h"


#include <string.h>

VirtualANode::VirtualANode(RenderEngine *renderengine, 
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
		int out)
 : VirtualNode(renderengine, 
 		vconsole, 
		real_module, 
		real_plugin,
		track, 
		parent_module, 
		input_is_master,
		output_is_master,
		in,
		out)
{
	for(int i = 0; i < RING_BUFFERS; i++)
	{
		this->buffer_in[i] = buffer_in[i];
		this->buffer_out[i] = buffer_out[i];
	}

	for(int i = 0; i < MAXCHANNELS; i++)
	{
		pan_before[i] = pan_after[i] = 0;
	}
}

VirtualANode::~VirtualANode()
{
	if(!shared_output) delete [] buffer_out[0];
	if(!shared_input) delete [] buffer_in[0];
}








VirtualNode* VirtualANode::create_module(Plugin *real_plugin, 
							Module *real_module, 
							Track *track)
{
	return new VirtualANode(renderengine, 
		vconsole, 
		real_module,
		0,
		track,
		this,
		data_in_input ? buffer_in : buffer_out,
		buffer_out,
		data_in_input ? input_is_master : output_is_master,
		output_is_master,
		real_plugin->in,
		real_plugin->out);
}


VirtualNode* VirtualANode::create_plugin(Plugin *real_plugin)
{
	return new VirtualANode(renderengine, 
		vconsole, 
		0,
		real_plugin,
		track,
		this,
		data_in_input ? buffer_in : buffer_out,
		buffer_out,
		data_in_input ? input_is_master : output_is_master,
		output_is_master,
		real_plugin->in,
		real_plugin->out);
}


void VirtualANode::new_output_buffer()
{
	buffer_out[0] = new double[renderengine->edl->session->audio_module_fragment];
}

void VirtualANode::new_input_buffer()
{
	buffer_in[0] = new double[renderengine->edl->session->audio_module_fragment];
}

double* VirtualANode::get_module_input(int ring_buffer, int64_t fragment_offset)
{
	double *result;
	if(data_in_input)
	{
		if(input_is_master) 
			result = buffer_in[ring_buffer] + fragment_offset;
		else
			result = buffer_in[0];
	}
	else
	{
		if(output_is_master)
			result = buffer_out[ring_buffer] + fragment_offset;
		else
			result = buffer_out[0];
	}
	return result;
}

double* VirtualANode::get_module_output(int ring_buffer, int64_t fragment_position)
{
	double *result;

	if(output_is_master)
		result = buffer_out[ring_buffer] + fragment_position;
	else
		result = buffer_out[0];

	return result;
}

int VirtualANode::render(double **audio_out, 
				int64_t audio_out_position, 
				int ring_buffer,
				int64_t fragment_position,
				int64_t fragment_len, 
				int64_t real_position, 
				int64_t source_length, 
				int reverse,
				ARender *arender)
{
	if(real_module)
	{
		render_as_module(audio_out, 
				audio_out_position, 
				ring_buffer,
				fragment_position,
				fragment_len, 
				real_position, 
				arender);
	}
	else
	if(real_plugin)
	{
//printf("VirtualANode::render 1\n");
		render_as_plugin(real_position,
				fragment_position,
				fragment_len,
				ring_buffer);
	}
	return 0;
}

void VirtualANode::render_as_plugin(int64_t real_position, 
	int64_t fragment_position, 
	int64_t fragment_len,
	int ring_buffer)
{
//printf("VirtualANode::render_as_plugin 1\n");
	((AAttachmentPoint*)attachment)->render(buffer_in[ring_buffer] + fragment_position, 
		buffer_out[ring_buffer] + fragment_position, 
		fragment_len, 
	  	real_position);
}

int VirtualANode::render_as_module(double **audio_out, 
				int64_t audio_out_position, 
				int ring_buffer,
				int64_t fragment_position,
				int64_t fragment_len, 
				int64_t real_position, 
				ARender *arender)
{
	int in_output = 0;
	double *buffer_in = get_module_input(ring_buffer, fragment_position);
	double *buffer_out = get_module_output(ring_buffer, fragment_position);
	int direction = renderengine->command->get_direction();

//printf("VirtualANode::render_as_module 1 %p\n", this->buffer_in[ring_buffer] + fragment_position);

// for(int i = 0; i < fragment_len; i++)
// {
// int16_t value = (int16_t)(buffer_in[i] * 32767);
// fwrite(&value, 2, 1, stdout);
// }

//printf("VirtualANode::render_as_module 1 %d\n", ring_buffer);
// Render fade
	render_fade(buffer_in, 
				buffer_out,
				fragment_len,
				real_position,
				track->automation->fade_autos);

// Get the peak but don't limit
//printf("VirtualANode::render_as_module 1 %p %d\n", real_module, renderengine->command->realtime);
	if(real_module && renderengine->command->realtime)
	{
		ARender *arender = ((VirtualAConsole*)vconsole)->arender;
		double max = 0, min = 0, peak;
		int64_t meter_render_start; // Starting sample of meter block
		int64_t meter_render_end;   // Ending sample of meter block
		int64_t current_level = ((AModule*)real_module)->current_level;

//printf("VirtualANode::render_as_module 1 %p %p\n", ((AModule*)real_module), ((AModule*)real_module)->level_samples);
// Scan fragment in meter sized fragments
		for(int i = 0; i < fragment_len; )
		{
			meter_render_start = i;
			meter_render_end = i + arender->meter_render_fragment;
			if(meter_render_end > fragment_len) meter_render_end = fragment_len;
			max = 0;
			min = 0;
			
			for( ; i < meter_render_end; i++)
			{
				if(buffer_out[i] > max) max = buffer_out[i];
				else
				if(buffer_out[i] < min) min = buffer_out[i];
			}

			if(fabs(max) > fabs(min)) 
				peak = fabs(max);
			else
				peak = fabs(min);

			((AModule*)real_module)->level_history[current_level] = 
				peak;
			((AModule*)real_module)->level_samples[current_level] = 
				(renderengine->command->get_direction() == PLAY_FORWARD) ?
				(real_position + meter_render_start) :
				(real_position - meter_render_start);
//printf("VirtualANode::render_as_module 2 %d\n", ((AModule*)real_module)->level_samples[current_level]);
			((AModule*)real_module)->current_level = 
				arender->get_next_peak(((AModule*)real_module)->current_level);
		}
	}

//printf("VirtualANode::render_as_module 1\n");
// process pans and copy the output to the output channels
// Keep rendering unmuted fragments until finished.
	int mute_constant;
	int64_t mute_fragment;
	int64_t mute_position = 0;

	for(int i = 0; i < fragment_len; )
	{
		mute_fragment = fragment_len - i;

// How many samples until the next mute?
		get_mute_fragment(real_position,
				mute_constant, 
				mute_fragment,
				(Autos*)track->automation->mute_autos);
//printf("VirtualANode::render_as_module 1\n");

// Fragment is playable
		if(!mute_constant)
		{
			for(int i = 0; 
				i < MAX_CHANNELS; 
				i++)
			{
//printf("VirtualANode::render_as_module %d\n", i);
				if(audio_out[i])
				{
					double *buffer = audio_out[i];

					render_pan(buffer_out + mute_position, 
								buffer + audio_out_position + mute_position,
								mute_fragment,
								real_position,
								(Autos*)track->automation->pan_autos,
								i);
				}
			}
		}

		fragment_len -= mute_fragment;
		real_position += (direction == PLAY_REVERSE) ? -mute_fragment : mute_fragment;
		mute_position += mute_fragment;
	}
//printf("VirtualANode::render_as_module 2\n");
	return 0;
}

int VirtualANode::render_fade(double *input,        // start of input fragment
								double *output,        // start of output fragment
								int64_t fragment_len,      // fragment length in input scale
								int64_t input_position, // starting sample of input buffer in project
								Autos *autos)
{
	double value, fade_value;
	int direction = renderengine->command->get_direction();
	FloatAuto *previous = 0;
	FloatAuto *next = 0;


	if(((FloatAutos*)autos)->automation_is_constant(input_position, 
		fragment_len,
		direction,
		fade_value))
	{
//printf("VirtualANode::render_fade 1 %d %f\n", input_position, fade_value);
		if(fade_value <= INFINITYGAIN) 
			value = 0;
		else
			value = DB::fromdb(fade_value);
		for(int64_t i = 0; i < fragment_len; i++)
		{
			output[i] = input[i] * value;
		}
	}
	else
	{
//printf("VirtualANode::render_fade 10 %d\n", input_position);
		for(int64_t i = 0; i < fragment_len; i++)
		{
			int64_t slope_len = fragment_len - i;

			fade_value = ((FloatAutos*)autos)->get_value(input_position, 
				direction,
				previous,
				next);

			if(fade_value <= INFINITYGAIN)
				value = 0;
			else
				value = DB::fromdb(fade_value);

			output[i] = input[i] * value;

			if(direction == PLAY_FORWARD)
				input_position++;

			else
				input_position--;
		}
	}

// Get slope intercept formula for next fragment
// 		get_fade_automation(slope, 
// 						intercept, 
// 						input_position,
// 						slope_len,
// 						autos);
// 
// 		if(slope != 0)
// 		{
// 			for(double j = 0; 
// 				j < slope_len; 
// 				j++, i++)
// 			{
// 				value = slope * j + intercept;
// 				value = DB::fromdb(value);
// 				output[i] = input[i] * value;
// 			}
// 		}
// 		else
// 		{
// 			double value = DB::fromdb(intercept);
// 			for(int j = 0; 
// 				j < slope_len; 
// 				j++, i++)
// 			{
// 				output[i] = input[i] * value;
// 			}
// 		}
// 
// 
// 		if(direction == PLAY_FORWARD)
// 			input_position += slope_len;
// 		else
// 			input_position -= slope_len;
// 	}

	return 0;
}

int VirtualANode::render_pan(double *input,        // start of input fragment
								double *output,        // start of output fragment
								int64_t fragment_len,      // fragment length in input scale
								int64_t input_position, // starting sample of input buffer in project
								Autos *autos,
								int channel)
{
	double slope, intercept;
	int direction = renderengine->command->get_direction();

	for(int64_t i = 0; i < fragment_len; )
	{
		int64_t slope_len = fragment_len - i;

// Get slope intercept formula for next fragment
		get_pan_automation(slope, 
						intercept, 
						input_position,
						slope_len,
						autos,
						channel);

		if(slope != 0)
		{
			for(double j = 0; j < slope_len; j++, i++)
			{
				value = slope * j + intercept;
				output[i] += input[i] * value;
			}
		}
		else
		{
			for(int j = 0; j < slope_len; j++, i++)
			{
				output[i] += input[i] * intercept;
			}
		}


		if(direction == PLAY_FORWARD)
			input_position += slope_len;
		else
			input_position -= slope_len;
	}

// if(channel == 0)
// for(int i = 0; i < fragment_len; i++)
// {
// int16_t value = (int16_t)(input[i] * 32767);
// fwrite(&value, 2, 1, stdout);
// }

	return 0;
}

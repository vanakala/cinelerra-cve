#include "aedit.h"
#include "amodule.h"
#include "arender.h"
#include "assets.h"
#include "atrack.h"
#include "audiodevice.h"
#include "condition.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "levelwindow.h"
#include "playabletracks.h"
#include "plugin.h"
#include "preferences.h"
#include "renderengine.h"
#include "thread.h"
#include "tracks.h"
#include "transportque.h"
#include "virtualaconsole.h"
#include "virtualanode.h"
#include "virtualnode.h"


VirtualAConsole::VirtualAConsole(RenderEngine *renderengine, ARender *arender)
 : VirtualConsole(renderengine, arender, TRACK_AUDIO)
{
	this->arender = arender;
	output_temp = 0;
	output_allocation = 0;
}

VirtualAConsole::~VirtualAConsole()
{
	if(output_temp) delete [] output_temp;
}


void VirtualAConsole::get_playable_tracks()
{
	if(!playable_tracks)
		playable_tracks = new PlayableTracks(renderengine, 
			commonrender->current_position, 
			TRACK_AUDIO,
			1);
}


VirtualNode* VirtualAConsole::new_entry_node(Track *track, 
	Module *module,
	int track_number)
{
	return new VirtualANode(renderengine,
		this, 
		module,
		0,
		track,
		0);
	return 0;
}



int VirtualAConsole::process_buffer(int64_t len,
	int64_t start_position,
	int last_buffer,
	int64_t absolute_position)
{
	int result = 0;


// clear output buffers
	for(int i = 0; i < MAX_CHANNELS; i++)
	{
// printf("VirtualAConsole::process_buffer 2 %d %p %lld\n", 
// i, 
// arender->audio_out[i],
// len);
		if(arender->audio_out[i])
		{
			bzero(arender->audio_out[i], len * sizeof(double));
		}
	}

// Create temporary output
	if(output_temp && output_allocation < len)
	{
		delete [] output_temp;
		output_temp = 0;
	}

	if(!output_temp)
	{
		output_temp = new double[len];
		output_allocation = len;
	}

// Reset plugin rendering status
	reset_attachments();
//printf("VirtualAConsole::process_buffer 1 %p\n", output_temp);

// Render exit nodes
	for(int i = 0; i < exit_nodes.total; i++)
	{
		VirtualANode *node = (VirtualANode*)exit_nodes.values[i];
		Track *track = node->track;

//printf("VirtualAConsole::process_buffer 2 %d %p\n", i, output_temp);
		result |= node->render(output_temp, 
			start_position + track->nudge,
			len,
			renderengine->edl->session->sample_rate);
//printf("VirtualAConsole::process_buffer 3 %p\n", output_temp);
	}
//printf("VirtualAConsole::process_buffer 4\n");


// get peaks and limit volume in the fragment
	for(int i = 0; i < MAX_CHANNELS; i++)
	{
		double *current_buffer = arender->audio_out[i];
		if(current_buffer)
		{

			for(int j = 0; j < len; )
			{
				int meter_render_end;
// Get length to test for meter and limit
				if(renderengine->command->realtime)
					meter_render_end = j + arender->meter_render_fragment;
				else
					meter_render_end = len;

				if(meter_render_end > len) 
					meter_render_end =  len;

				double peak = 0;

				for( ; j < meter_render_end; j++)
				{
// Level history comes before clipping to get over status
					double *sample = &current_buffer[j];


					if(fabs(*sample) > peak) peak = fabs(*sample);
// Make the output device clip it
// 					if(*sample > 1) *sample = 1;
// 					else
// 					if(*sample < -1) *sample = -1;
				}


 				if(renderengine->command->realtime)
 				{
					arender->level_history[i][arender->current_level[i]] = peak;
					arender->level_samples[arender->current_level[i]] = 
						renderengine->command->get_direction() == PLAY_REVERSE ? 
						start_position - j : 
						start_position + j;
 					arender->current_level[i] = arender->get_next_peak(arender->current_level[i]);
 				}
			}
		}
	}



//printf("VirtualAConsole::process_buffer 5\n");


// Pack channels, fix speed and send to device.
	if(renderengine->command->realtime && !interrupt)
	{
// speed parameters
// length compensated for speed
		int real_output_len;
// output sample
		double sample;
		int k;
		double *audio_out_packed[MAX_CHANNELS];

		for(int i = 0, j = 0; i < MAX_CHANNELS; i++)
		{
			if(renderengine->config->aconfig->do_channel[i])
			{
				audio_out_packed[j++] = arender->audio_out[i];
			}
		}

		for(int i = 0; 
			i < renderengine->config->aconfig->total_playable_channels(); 
			i++)
		{
			int in, out;
			int fragment_end;

			double *current_buffer = audio_out_packed[i];

// Time stretch the fragment to the real_output size
			if(renderengine->command->get_speed() > 1)
			{
// Number of samples in real output buffer for each to sample rendered.
				int interpolate_len = (int)renderengine->command->get_speed();
				for(in = 0, out = 0; in < len; )
				{
					sample = 0;
					for(k = 0; k < interpolate_len; k++)
					{
						sample += current_buffer[in++];
					}
					sample /= renderengine->command->get_speed();
					current_buffer[out++] = sample;
				}
				real_output_len = out;
			}
			else
			if(renderengine->command->get_speed() < 1)
			{
// number of samples to skip
 				int interpolate_len = (int)(1.0 / renderengine->command->get_speed());
				real_output_len = len * interpolate_len;

				for(in = len - 1, out = real_output_len - 1; in >= 0; )
				{
					for(k = 0; k < interpolate_len; k++)
					{
						current_buffer[out--] = current_buffer[in];
					}
					in--;
				}
			}
			else
				real_output_len = len;
		}

// Wait until video is ready
		if(arender->first_buffer)
		{
			renderengine->first_frame_lock->lock("VirtualAConsole::process_buffer");
			arender->first_buffer = 0;
		}
		if(!renderengine->audio->get_interrupted())
		{
			renderengine->audio->write_buffer(audio_out_packed, 
				real_output_len, 
				renderengine->config->aconfig->total_playable_channels());
		}

		if(renderengine->audio->get_interrupted()) interrupt = 1;
	}




//printf("VirtualAConsole::process_buffer 100\n");




	return result;
}

























int VirtualAConsole::init_rendering(int duplicate)
{
	return 0;
}


int VirtualAConsole::send_last_output_buffer()
{
	renderengine->audio->set_last_buffer();
	return 0;
}


#include "aedit.h"
#include "amodule.h"
#include "arender.h"
#include "assets.h"
#include "atrack.h"
#include "audiodevice.h"
#include "cache.h"
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
//printf("VirtualAConsole::VirtualAConsole\n");
	this->arender = arender;
}

VirtualAConsole::~VirtualAConsole()
{
	for(int i = 0; i < ring_buffers; i++)
	{
		delete_input_buffer(i);
	}
}

int VirtualAConsole::total_ring_buffers()
{
	return renderengine->command->realtime ? 2 : 1;
}

void VirtualAConsole::get_playable_tracks()
{
	if(!playable_tracks)
		playable_tracks = new PlayableTracks(renderengine, 
			commonrender->current_position, 
			TRACK_AUDIO);
}

void VirtualAConsole::new_input_buffer(int ring_buffer)
{
	buffer_in[ring_buffer] = new double*[total_tracks];
	for(int i = 0; i < total_tracks; i++)
	{
		buffer_in[ring_buffer][i] = new double[renderengine->edl->session->audio_read_length];
	}
}

void VirtualAConsole::delete_input_buffer(int ring_buffer)
{
	for(int i = 0; i < total_tracks; i++)
	{
		delete [] buffer_in[ring_buffer][i];
	}
	delete [] buffer_in[ring_buffer];
}

VirtualNode* VirtualAConsole::new_toplevel_node(Track *track, 
	Module *module,
	int track_number)
{
	double *track_buffer[RING_BUFFERS];
//printf("VirtualAConsole::new_toplevel_node %p\n", module);
	for(int i = 0; i < total_ring_buffers(); i++)
		track_buffer[i] = buffer_in[i][track_number];
	return new VirtualANode(renderengine,
		this, 
		module,
		0,
		track,
		0,
		track_buffer,
		track_buffer,
		1,
		1,
		1,
		1);
	return 0;
}

int VirtualAConsole::stop_rendering(int duplicate)
{
	if(renderengine->command->realtime)
	{
		Thread::join();
	}
	return 0;
}


int VirtualAConsole::process_buffer(long input_len,
	long input_position,
	int last_buffer,
	long absolute_position)
{
	int result = 0;
// wait for an input_buffer to become available
	if(renderengine->command->realtime)
		output_lock[current_input_buffer]->lock();

	if(!interrupt)
	{
// Load tracks
		double **buffer_in = this->buffer_in[current_input_buffer];

		for(int i = 0; i < total_tracks; i++)
		{
			result |= ((AModule*)virtual_modules[i]->real_module)->render(buffer_in[i],
				input_len, 
				input_position,
				renderengine->command->get_direction());
		}



		this->input_len[current_input_buffer] = input_len;
		this->input_position[current_input_buffer] = input_position;
		this->last_playback[current_input_buffer] = last_buffer;
		this->last_reconfigure[current_input_buffer] = 0;
		this->absolute_position[current_input_buffer] = absolute_position;

		if(renderengine->command->realtime)
			input_lock[current_input_buffer]->unlock();
		else
			process_console();


//printf("VirtualAConsole::process_buffer 5 %p\n", buffer_in[0]);
// for(int i = 0; i < input_len; i++)
// {
// int16_t value = (int16_t)(buffer_in[0][i] * 32767);
// fwrite(&value, 2, 1, stdout);
// }


		swap_input_buffer();
	}
	return result;
}

void VirtualAConsole::process_console()
{
	int i, j, k;
// length and lowest numbered sample of fragment in input buffer
	int buffer = current_vconsole_buffer;
	long fragment_len, fragment_position;	
// generic buffer
	double *current_buffer;
// starting sample of fragment in project
	long real_position;
// info for meters
	double min, max, peak;
	long meter_render_end;    // end of current meter fragment for getting levels
	long current_fragment_peak; // first meter peak in fragment
	long input_len = this->input_len[buffer];
	long input_position = this->input_position[buffer];
	long absolute_position = this->absolute_position[buffer];



//printf("VirtualAConsole::process_console 1 %p\n", this->buffer_in[buffer][0]);

// process entire input buffer by filling one output buffer at a time
	for(fragment_position = 0; 
		fragment_position < input_len && !interrupt; )
	{

// test for end of input buffer
		fragment_len = renderengine->edl->session->audio_module_fragment;
		if(fragment_position + fragment_len > input_len)
			fragment_len = input_len - fragment_position;




// clear output buffers
		for(i = 0; i < MAX_CHANNELS; i++)
		{
			if(arender->audio_out[i])
			{
				bzero(arender->audio_out[i], fragment_len * sizeof(double));
			}
		}





// get the start of the fragment in the project
		real_position = 
			(renderengine->command->get_direction() == PLAY_REVERSE) ? 
				input_position - fragment_position : 
				input_position + fragment_position;

// render nodes in sorted list
		for(i = 0; i < render_list.total; i++)
		{
//printf("VirtualAConsole::process_console 1 %p\n", this->buffer_in[buffer][i] + fragment_position);
			((VirtualANode*)render_list.values[i])->render(arender->audio_out, 
					0, 
					buffer,
					fragment_position,
					fragment_len, 
					real_position, 
					arender->source_length,
					renderengine->reverse,
					arender);
		}

// get peaks and limit volume in the fragment
		for(i = 0; i < MAX_CHANNELS; i++)
		{
			current_buffer = arender->audio_out[i];
			if(current_buffer)
			{

				for(j = 0; j < fragment_len; )
				{
// Get length to test for meter
					if(renderengine->command->realtime)
						meter_render_end = j + arender->meter_render_fragment;
					else
						meter_render_end = fragment_len;

					if(meter_render_end > fragment_len) 
						meter_render_end =  fragment_len;

					min = max = 0;

					for( ; j < meter_render_end; j++)
					{
// Level history comes before clipping to get over status
						if(current_buffer[j] > max) max = current_buffer[j];
						else
						if(current_buffer[j] < min) min = current_buffer[j];

						if(current_buffer[j] > 1) current_buffer[j] = 1;
						else
						if(current_buffer[j] < -1) current_buffer[j] = -1;
					}


					if(fabs(max) > fabs(min))
						peak = fabs(max);
					else
						peak = fabs(min);

 					if(renderengine->command->realtime)
 					{
						arender->level_history[i][arender->current_level[i]] = peak;
						arender->level_samples[arender->current_level[i]] = 
							(renderengine->command->get_direction() == PLAY_REVERSE) ? 
							real_position - j : 
							real_position + j;
 						arender->current_level[i] = arender->get_next_peak(arender->current_level[i]);
 					}
				}

			}
		}


// advance fragment
		fragment_position += fragment_len;

// Pack channels, fix speed and send to device.
		if(renderengine->command->realtime && !interrupt)
		{
// speed parameters
			long real_output_len; // length compensated for speed
			double sample;       // output sample
			int k;
			double *audio_out_packed[MAX_CHANNELS];

			for(i = 0, j = 0; i < MAX_CHANNELS; i++)
			{
				if(renderengine->config->aconfig->do_channel[i])
				{
					audio_out_packed[j++] = arender->audio_out[i];
				}
			}

			for(i = 0; 
				i < renderengine->config->aconfig->total_playable_channels(); 
				i++)
			{
				long in, out;
				long fragment_end;

				current_buffer = audio_out_packed[i];

// Time stretch the fragment to the real_output size
				if(renderengine->command->get_speed() > 1)
				{
// Number of samples in real output buffer for each to sample rendered.
					int interpolate_len = (int)renderengine->command->get_speed();
					for(in = 0, out = 0; in < fragment_len; )
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
					int interpolate_len = (int)(1 / renderengine->command->get_speed()); // number of samples to skip
					real_output_len = fragment_len * interpolate_len;

					for(in = fragment_len - 1, out = real_output_len - 1; in >= 0; )
					{
						for(k = 0; k < interpolate_len; k++)
						{
							current_buffer[out--] = current_buffer[in];
						}
						in--;
					}
				}
				else
					real_output_len = fragment_len;
			}

			if(!renderengine->audio->get_interrupted())
			{
				renderengine->audio->write_buffer(audio_out_packed, 
					real_output_len, 
					renderengine->config->aconfig->total_playable_channels());
			}

			if(renderengine->audio->get_interrupted()) interrupt = 1;
		}


// for(int i = 0; i < fragment_len; i++)
// {
// int16_t value = (int16_t)(arender->audio_out[0][i] * 32767);
// fwrite(&value, 2, 1, stdout);
// }
	}
}


void VirtualAConsole::run()
{
	startup_lock->unlock();

	while(!done && !interrupt)
	{
// wait for a buffer to render through console
		input_lock[current_vconsole_buffer]->lock();

		if(!done && !interrupt && !last_reconfigure[current_vconsole_buffer])
		{
// render it if not last buffer
// send to output device or the previously set output buffer
			process_console();

// test for exit conditions tied to the buffer
			if(last_playback[current_vconsole_buffer]) done = 1;

// free up buffer for reading from disk
			output_lock[current_vconsole_buffer]->unlock();

// get next buffer
			if(!done) swap_thread_buffer();
		}
		else
		if(last_reconfigure[current_vconsole_buffer])
			done = 1;
	}

	if(interrupt)
	{
		for(int i = 0; i < total_ring_buffers(); i++)
		{
			output_lock[i]->unlock();
		}
	}
	else
	if(!last_reconfigure[current_vconsole_buffer])
	{
		if(renderengine->command->realtime)
			send_last_output_buffer();
	}
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


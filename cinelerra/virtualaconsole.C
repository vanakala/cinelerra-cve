
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

#include "aedit.h"
#include "aframe.h"
#include "amodule.h"
#include "arender.h"
#include "assets.h"
#include "atrack.h"
#include "audiodevice.h"
#include "bcsignals.h"
#include "clip.h"
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
	for(int i = 0; i < MAX_CHANNELS; i++)
		audio_out_packed[i] = 0;
	packed_buffer_len = 0;
}

VirtualAConsole::~VirtualAConsole()
{
	if(output_temp) delete output_temp;
	for(int i = 0; i < MAX_CHANNELS; i++)
	{
		if(audio_out_packed[i])
		{
			delete [] audio_out_packed[i];
		}
	}
}


void VirtualAConsole::get_playable_tracks()
{
	if(!playable_tracks)
		playable_tracks = new PlayableTracks(renderengine, 
			commonrender->current_postime,
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
}



int VirtualAConsole::process_buffer(int len,
	ptstime start_postime,
	int last_buffer)
{
	int result = 0;

// clear output buffers
	for(int i = 0; i < MAX_CHANNELS; i++)
	{
		if(arender->audio_out[i])
		{
			arender->audio_out[i]->init_aframe(start_postime, len);
		}
	}

// Create temporary output
	if(!output_temp)
		output_temp = new AFrame(len);

// Reset plugin rendering status
	reset_attachments();

// Render exit nodes
	for(int i = 0; i < exit_nodes.total; i++)
	{
		VirtualANode *node = (VirtualANode*)exit_nodes.values[i];
		output_temp->init_aframe(start_postime + node->track->nudge, len);
		output_temp->source_length = len;
		output_temp->samplerate = renderengine->edl->session->sample_rate;
		output_temp->channel = i;
		node->render(output_temp);
	}


// get peaks and limit volume in the fragment
	for(int i = 0; i < MAX_CHANNELS; i++)
	{
		AFrame *current_aframe = arender->audio_out[i];

		if(current_aframe)
		{
			samplenum start_position = arender->tounits(start_postime, 1);
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
					double *sample = &current_aframe->buffer[j];


					if(fabs(*sample) > peak) 
						peak = fabs(*sample);
				}


				if(renderengine->command->realtime)
				{
					arender->level_history[i][arender->current_level[i]] = peak;
					arender->level_samples[arender->current_level[i]] = 
						start_position + j;
					arender->current_level[i] = arender->get_next_peak(arender->current_level[i]);
				}
			}
		}
	}


// Pack channels, fix speed and send to device.
	if(renderengine->command->realtime && !interrupt)
	{
// speed parameters
// length compensated for speed
		int real_output_len;
// output sample
		double sample;
		int k;
		double *audio_buf[MAX_CHANNELS];
		double **in_process;
		int audio_channels = renderengine->edl->session->audio_channels;
		int direction = renderengine->command->get_direction();
		float speed = renderengine->command->get_speed();
		if(!EQUIV(speed, 1))
		{
			int reqlen = round((double) len / speed);
			if(reqlen > packed_buffer_len){
				for(int i = 0; i < MAX_CHANNELS; i++)
				{
					if(audio_out_packed[i])
					{
						delete [] audio_out_packed[i];
						audio_out_packed[i] = 0;
					}
				}
				for(int i = 0; i < audio_channels; i++)
					audio_out_packed[i] = new double[reqlen];
				packed_buffer_len = reqlen;
			}
			in_process = audio_out_packed;
		}

		for(int i = 0; 
			i < audio_channels; 
			i++)
		{
			int in, out;
			int fragment_end;
			double *current_buffer, *orig_buffer;

			orig_buffer = audio_buf[i] = arender->audio_out[i]->buffer;
			current_buffer = audio_out_packed[i];

// Time stretch the fragment to the real_output size
			if(speed > 1)
			{
// Number of samples in real output buffer for each to sample rendered.
				int interpolate_len = (int)speed;
				if(direction == PLAY_FORWARD)
				{
					for(in = 0, out = 0; in < len; )
					{
						sample = 0;
						for(k = 0; k < interpolate_len; k++)
						{
							sample += orig_buffer[in++];
						}
						sample /= speed;
						current_buffer[out++] = sample;
					}
				}
				else
				{
					for(in = len - 1, out = 0; in >= 0; )
					{
						sample = 0;
						for(k = 0; k < interpolate_len; k++)
						{
							sample += orig_buffer[in--];
						}
						sample /= speed;
						current_buffer[out++] = sample;
					}
				}
				real_output_len = out;
			}
			else
			if(speed < 1)
			{
// number of samples to skip
				int interpolate_len = (int)(1.0 / speed);
				real_output_len = len * interpolate_len;
				if(direction == PLAY_FORWARD)
				{
					for(in = 0, out = 0; in < len; )
					{
						for(k = 0; k < interpolate_len; k++)
						{
							current_buffer[out++] = orig_buffer[in];
						}
						in++;
					}
				}
				else
				{
					for(in = len - 1, out = 0; in >= 0; )
					{
						for(k = 0; k < interpolate_len; k++)
						{
							current_buffer[out++] = orig_buffer[in];
						}
						in--;
					}
				}
			}
			else
			{
				if(direction == PLAY_REVERSE)
				{
					for(int s = 0, e = len - 1; e > s; e--, s++)
					{
						sample = orig_buffer[s];
						orig_buffer[s] = orig_buffer[e];
						orig_buffer[e] = sample;
					}
				}
				real_output_len = len;
				in_process = audio_buf;
			}
		}

// Wait until video is ready
		if(arender->first_buffer)
		{
			renderengine->first_frame_lock->lock("VirtualAConsole::process_buffer");
			arender->first_buffer = 0;
		}
		if(!renderengine->audio->get_interrupted())
		{
			renderengine->audio->write_buffer(in_process,
				real_output_len);
		}

		if(renderengine->audio->get_interrupted()) interrupt = 1;
	}
	return result;
}

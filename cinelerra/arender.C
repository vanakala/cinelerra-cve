
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

#include "amodule.h"
#include "arender.h"
#include "atrack.h"
#include "atrack.h"
#include "audiodevice.h"
#include "auto.h"
#include "autos.h"
#include "bcsignals.h"
#include "cache.h"
#include "condition.h"
#include "edit.h"
#include "edl.h"
#include "edlsession.h"
#include "levelwindow.h"
#include "mainsession.h"
#include "playabletracks.h"
#include "playbackengine.h"
#include "preferences.h"
#include "renderengine.h"
#include "tracks.h"
#include "transportque.h"
#include "virtualaconsole.h"
#include "virtualconsole.h"
#include "virtualnode.h"

ARender::ARender(RenderEngine *renderengine)
 : CommonRender(renderengine)
{
// Clear output buffers
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		audio_out[i] = 0;
		level_history[i] = 0;
	}
	level_samples = 0;
	total_peaks = 0;

	data_type = TRACK_AUDIO;
}

ARender::~ARender()
{
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		if(audio_out[i]) delete [] audio_out[i];
		if(level_history[i]) delete [] level_history[i];
	}
	if(level_samples) delete [] level_samples;
}

void ARender::arm_command()
{
// Need the meter history now so AModule can allocate its own history
	calculate_history_size();
	CommonRender::arm_command();
	asynchronous = 1;
	init_meters();
}


int ARender::get_total_tracks()
{
	return renderengine->edl->tracks->total_audio_tracks();
}

Module* ARender::new_module(Track *track)
{
	return new AModule(renderengine, this, 0, track);
}

int ARender::calculate_history_size()
{
	if(total_peaks > 0)
		return total_peaks;
	else
	{
		meter_render_fragment = renderengine->fragment_len;
// This number and the timer in tracking.C determine the rate
		while(meter_render_fragment > 
			renderengine->edl->session->sample_rate / TRACKING_RATE) 
			meter_render_fragment /= 2;
		total_peaks = 16 * 
			renderengine->fragment_len / 
			meter_render_fragment;
		return total_peaks;
	}
}

int ARender::init_meters()
{
// not providing enough peaks results in peaks that are ahead of the sound
	if(level_samples) delete [] level_samples;
	calculate_history_size();
	level_samples = new int64_t[total_peaks];

	for(int i = 0; i < MAXCHANNELS;i++)
	{
		current_level[i] = 0;
		if(audio_out[i] && !level_history[i]) 
			level_history[i] = new double[total_peaks];
	}

	for(int i = 0; i < total_peaks; i++)
	{
		level_samples[i] = -1;
	}
	
	for(int j = 0; j < MAXCHANNELS; j++)
	{
		if(audio_out[j]) 
			for(int i = 0; i < total_peaks; i++)
				level_history[j][i] = 0;
	}
	return 0;
}

void ARender::init_output_buffers()
{
	if(renderengine->command->realtime)
	{
		for(int i = 0; i < MAXCHANNELS; i++)
		{
// Reset the output buffers in case speed changed
			delete [] audio_out[i];
			audio_out[i] = 0;

			if(i < renderengine->edl->session->audio_channels)
			{
				audio_out[i] = new double[renderengine->adjusted_fragment_len];
			}
		}
	}
}


VirtualConsole* ARender::new_vconsole_object() 
{ 
	return new VirtualAConsole(renderengine, this);
}

posnum ARender::tounits(ptstime position, int roundfl)
{
	if(roundfl)
		return round(position * renderengine->edl->session->sample_rate);
	else
		return (posnum)(position * renderengine->edl->session->sample_rate);
}

ptstime ARender::fromunits(posnum position)
{
	return (double)position / renderengine->edl->session->sample_rate;
}


int ARender::process_buffer(double **buffer_out, 
	int input_len, 
	ptstime input_postime,
	int last_buffer)
{
	int result = 0;
	this->last_playback = last_buffer;
	int fragment_position = 0;
	posnum fragment_len = input_len;
	int reconfigure = 0;
	current_postime = input_postime;
	current_position = tounits(current_position, 1);

	while(fragment_position < input_len)
	{
		for(int i = 0; i < MAXCHANNELS; i++)
		{
			if(buffer_out[i])
				this->audio_out[i] = buffer_out[i] + fragment_position;
			else
				this->audio_out[i] = 0;
		}

		fragment_len = input_len;
		if(fragment_position + fragment_len > input_len)
			fragment_len = input_len - fragment_position;

		ptstime len_pts = fromunits(fragment_len);
		reconfigure = vconsole->test_reconfigure(len_pts,
			last_playback);
		fragment_len = tounits(len_pts);

		if(reconfigure) restart_playback();

		result = process_buffer(fragment_len, input_postime);

		fragment_position += fragment_len;
		input_postime += fromunits(fragment_len);
		current_postime = input_postime;
		current_position = tounits(current_postime, 1);
	}

// Don't delete audio_out on completion
	memset(this->audio_out, 0, sizeof(double*) * MAXCHANNELS);


	return result;
}


int ARender::process_buffer(int input_len, ptstime input_postime)
{
	int result = ((VirtualAConsole*)vconsole)->process_buffer(input_len,
		input_postime,
		last_playback);

// advance counters
	session_position += input_len;
	return result;
}

int ARender::get_history_number(int64_t *table, samplenum position)
{
// Get the entry closest to position
	int result = 0;
	int64_t min_difference = 0x7fffffff;
	for(int i = 0; i < total_peaks; i++)
	{
		if(labs(table[i] - position) < min_difference)
		{
			min_difference = labs(table[i] - position);
			result = i;
		}
	}
	return result;
}

void ARender::send_last_buffer()
{
	renderengine->audio->set_last_buffer();
}

int ARender::wait_device_completion()
{
// audio device should be entirely cleaned up by vconsole
	renderengine->audio->wait_for_completion();
	return 0;
}

void ARender::run()
{
	posnum current_input_length;
	int reconfigure = 0;

	first_buffer = 1;
	start_lock->unlock();

	while(!done && !interrupt && !last_playback)
	{
		current_input_length = renderengine->fragment_len;

		get_boundaries(current_input_length);

		if(current_input_length)
		{
			ptstime len_pts = fromunits(current_input_length);
			reconfigure = vconsole->test_reconfigure(len_pts,
				last_playback);
			current_input_length = tounits(len_pts, 1);

			if(reconfigure) restart_playback();
		}


// Update tracking if no video is playing.
		if(renderengine->command->realtime && 
			renderengine->playback_engine &&
			!renderengine->do_video)
		{
			double position = (double)renderengine->audio->current_position() / 
				renderengine->edl->session->sample_rate * 
				renderengine->command->get_speed();

			if(renderengine->command->get_direction() == PLAY_FORWARD) 
				position += renderengine->command->playbackstart;
			else
				position = renderengine->command->playbackstart - position;

// This number is not compensated for looping.  It's compensated in 
// PlaybackEngine::get_tracking_position when interpolation also happens.
			renderengine->playback_engine->update_tracking(position);
		}

		process_buffer(current_input_length, current_postime);

		advance_position(current_input_length);

		if(vconsole->interrupt) interrupt = 1;
	}

	if(!interrupt) send_last_buffer();
	if(renderengine->command->realtime) wait_device_completion();
	vconsole->stop_rendering(0);
	stop_plugins();
}



int ARender::get_datatype()
{
	return TRACK_AUDIO;
}

int ARender::reverse_buffer(double *buffer, int len)
{
	register int start, end;
	double temp;

	for(start = 0, end = len - 1; end > start; start++, end--)
	{
		temp = buffer[start];
		buffer[start] = buffer[end];
		buffer[end] = temp;
	}
}

int ARender::get_next_peak(int current_peak)
{
	current_peak++;
	if(current_peak >= total_peaks) current_peak = 0;
	return current_peak;
}

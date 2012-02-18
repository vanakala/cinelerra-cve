
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

#include "aframe.h"
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
		if(audio_out[i]) delete audio_out[i];
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
			renderengine->edl->session->sample_rate / TRACKING_RATE_DEFAULT) 
			meter_render_fragment /= 2;
		total_peaks = 16 * 
			renderengine->fragment_len / 
			meter_render_fragment;
		return total_peaks;
	}
}

void ARender::init_meters()
{
// not providing enough peaks results in peaks that are ahead of the sound
	if(level_samples) delete [] level_samples;
	calculate_history_size();
	level_samples = new samplenum[total_peaks];

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
}

void ARender::init_output_buffers()
{
	if(renderengine->command->realtime)
	{
		for(int i = 0; i < MAXCHANNELS; i++)
		{
// Reset the output buffers in case speed changed
			if(i < renderengine->edl->session->audio_channels)
			{
				if(audio_out[i])
					audio_out[i]->check_buffer(renderengine->fragment_len);
				else
					audio_out[i] = new AFrame(renderengine->fragment_len);
				audio_out[i]->samplerate = renderengine->edl->session->sample_rate;
				audio_out[i]->channel = i;
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


int ARender::process_buffer(AFrame **buffer_out)
{
	int result = 0;
	int fragment_len = buffer_out[0]->source_length;
	int samplerate = buffer_out[0]->samplerate;
	current_postime = buffer_out[0]->pts;
	ptstime fragment_duration = (ptstime)fragment_len / samplerate;
	ptstime fragment_end = current_postime + fragment_duration;

	last_playback = 0;

	for(int i = 0; i < MAXCHANNELS; i++)
	{
		if(buffer_out[i])
			this->audio_out[i] = buffer_out[i];
		else
			this->audio_out[i] = 0;
	}
	while(current_postime < fragment_end)
	{
		if(vconsole->test_reconfigure(fragment_duration, last_playback))
			restart_playback();

		fragment_len = round(fragment_duration * samplerate);
		result = process_buffer(fragment_len, current_postime);

		current_postime += fragment_duration;
	}

// Don't delete audio_out on completion
	memset(this->audio_out, 0, sizeof(AFrame*) * MAXCHANNELS);

	return result;
}


int ARender::process_buffer(int input_len, ptstime input_postime)
{
	int result = ((VirtualAConsole*)vconsole)->process_buffer(input_len,
		input_postime,
		last_playback);

	return result;
}

int ARender::get_history_number(samplenum *table, samplenum position)
{
// Get the entry closest to position
	int result = 0;
	samplenum min_difference = 0x7fffffff;
	for(int i = 0; i < total_peaks; i++)
	{
		if(llabs(table[i] - position) < min_difference)
		{
			min_difference = llabs(table[i] - position);
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
	samplenum current_input_length;
	ptstime current_input_duration;
	int reconfigure = 0;
	int revert = 0;

	first_buffer = 1;
	start_lock->unlock();

	while(!done && !interrupt && !last_playback)
	{
		current_input_length = renderengine->fragment_len;
		current_input_duration = fromunits(current_input_length);

		get_boundaries(current_input_duration, EPSILON);

		if(current_input_duration)
		{
			ptstime duration;
			duration = current_input_duration;
			reconfigure = vconsole->test_reconfigure(current_input_duration,
				last_playback);
			if(duration - current_input_duration > EPSILON && 
				renderengine->command->get_direction() == PLAY_REVERSE)
			{
// Reverse playback: play end of buffer
				current_postime += current_input_duration;
				current_input_duration = duration - current_input_duration;
				revert = 1;
			}
			current_input_length = tounits(current_input_duration, 1);

			if(reconfigure) restart_playback();
		}

// Update tracking if no video is playing.
		if(renderengine->command->realtime && 
			renderengine->playback_engine &&
			!renderengine->do_video)
		{
			ptstime position = (double)renderengine->audio->current_postime() *
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

		if(revert)
		{
			current_input_length = renderengine->fragment_len;
			current_input_duration = fromunits(current_input_length);
			revert = 0;
		}
		advance_position(audio_out[0]->pts, current_input_duration);

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

int ARender::get_next_peak(int current_peak)
{
	current_peak++;
	if(current_peak >= total_peaks) current_peak = 0;
	return current_peak;
}

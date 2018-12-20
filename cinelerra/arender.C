
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
#include "levelhist.h"
#include "levelwindow.h"
#include "mainsession.h"
#include "playabletracks.h"
#include "playbackengine.h"
#include "preferences.h"
#include "renderengine.h"
#include "tracks.h"
#include "transportcommand.h"
#include "virtualaconsole.h"
#include "virtualconsole.h"
#include "virtualnode.h"

ARender::ARender(RenderEngine *renderengine)
 : CommonRender(renderengine)
{
	for(int i = 0; i < MAXCHANNELS; i++)
		audio_out[i] = 0;

	output_levels = new LevelHistory();
	data_type = TRACK_AUDIO;
}

ARender::~ARender()
{
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		if(audio_out[i]) delete audio_out[i];
	}
}

void ARender::arm_command()
{
	CommonRender::arm_command();

	output_levels->reset(renderengine->fragment_len,
		edlsession->sample_rate, get_total_tracks());
}


int ARender::get_total_tracks()
{
	return renderengine->edl->tracks->total_audio_tracks();
}

Module* ARender::new_module(Track *track)
{
	return new AModule(renderengine, this, 0, track);
}

void ARender::init_output_buffers()
{
	if(renderengine->command->realtime)
	{
		for(int i = 0; i < MAXCHANNELS; i++)
		{
// Reset the output buffers in case speed changed
			if(i < edlsession->audio_channels)
			{
				if(audio_out[i])
					audio_out[i]->check_buffer(renderengine->fragment_len);
				else
					audio_out[i] = new AFrame(renderengine->fragment_len);
				audio_out[i]->samplerate = edlsession->sample_rate;
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
		return round(position * edlsession->sample_rate);
	else
		return (posnum)(position * edlsession->sample_rate);
}

ptstime ARender::fromunits(posnum position)
{
	return (double)position / edlsession->sample_rate;
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
	while(current_postime < fragment_end - EPSILON)
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

void ARender::send_last_buffer()
{
	renderengine->audio->set_last_buffer();
}

void ARender::wait_device_completion()
{
// audio device should be entirely cleaned up by vconsole
	renderengine->audio->wait_for_completion();
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

		process_buffer(current_input_length, current_postime);

		if(revert)
		{
			current_input_length = renderengine->fragment_len;
			current_input_duration = fromunits(current_input_length);
			revert = 0;
		}
		first_buffer = advance_position(audio_out[0]->pts, current_input_duration);
		if(vconsole->interrupt) interrupt = 1;
	}
	ptstime last_pts;
	if(!interrupt)
	{
		send_last_buffer();
		last_pts = current_postime;
	}
	if(renderengine->command->realtime) wait_device_completion();

	if(interrupt)
	{
		last_pts = renderengine->audio->current_postime() *
			renderengine->command->get_speed();
		if(renderengine->command->get_direction() == PLAY_FORWARD) 
			last_pts += renderengine->command->playbackstart;
		else
			last_pts = renderengine->command->playbackstart - last_pts;
	}

	renderengine->stop_tracking(last_pts, TRACK_AUDIO);

	renderengine->render_start_lock->unlock();
	stop_plugins();
}

int ARender::get_datatype()
{
	return TRACK_AUDIO;
}

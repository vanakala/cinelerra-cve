
/*
 * CINELERRA
 * Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>
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
#include "asset.h"
#include "atrackrender.h"
#include "audiodevice.h"
#include "audiorender.h"
#include "bcsignals.h"
#include "clip.h"
#include "condition.h"
#include "edl.h"
#include "edit.h"
#include "edlsession.h"
#include "file.h"
#include "levelhist.h"
#include "mainerror.h"
#include "renderbase.h"
#include "renderengine.inc"
#include "track.h"
#include "tracks.h"

AudioRender::AudioRender(RenderEngine *renderengine, EDL *edl)
 : RenderBase(renderengine, edl)
{
	output_levels = new LevelHistory();

	for(int i = 0; i < MAXCHANNELS; i++)
	{
		audio_out[i] = 0;
		audio_out_packed[i] = 0;
	}
	packed_buffer_len = 0;
}

AudioRender::~AudioRender()
{
	delete output_levels;
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		delete audio_out[i];
		delete [] audio_out_packed[i];
	}
	input_frames.remove_all_objects();
}

void AudioRender::init_frames()
{
	if(render_realtime)
	{
		out_channels = edl->this_edlsession->audio_channels;
		out_length = renderengine->fragment_len;
		out_samplerate = edl->this_edlsession->sample_rate;

		for(int i = 0; i < MAXCHANNELS; i++)
		{
			if(i < edl->this_edlsession->audio_channels)
			{
				if(audio_out[i])
					audio_out[i]->check_buffer(out_length);
				else
					audio_out[i] = new AFrame(out_length);
				audio_out[i]->samplerate = out_samplerate;
				audio_out[i]->channel = i;
			}
		}
	}
	output_levels->reset(out_length, out_samplerate, out_channels);
}

int AudioRender::process_buffer(AFrame **buffer_out)
{
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		if(buffer_out[i])
			audio_out[i] = buffer_out[i];
		else
			audio_out[i] = 0;
	}
	process_frames();
	// Not ours, must not delete them
	for(int i = 0; i < MAXCHANNELS; i++)
		audio_out[i] = 0;
	return 0;
}

ptstime AudioRender::calculate_render_duration()
{
	ptstime render_duration = audio_out[0]->to_duration(renderengine->fragment_len);

	if(render_direction == PLAY_FORWARD)
	{
		if(render_pts + render_duration >= render_end)
			render_duration = render_end - render_pts;
	}
	else
	{
		if(render_pts - render_duration <= render_start)
			render_duration = render_pts - render_start;
	}
	return render_duration;
}

void AudioRender::run()
{
	ptstime input_duration = 0;
	int first_buffer = 1;
	int real_output_len;
	double sample;
	double *audio_buf[MAX_CHANNELS];
	double **in_process;
	int audio_channels = edl->this_edlsession->audio_channels;

	start_lock->unlock();

	while(1)
	{
		// rehkendada palju vaja on (up to plugin/auto/edit)
		input_duration = calculate_render_duration();

		get_aframes(render_pts, input_duration);

		output_levels->fill(audio_out);

		if(!EQUIV(render_speed, 1))
		{
			int rqlen = round(
				(double) audio_out[0]->to_samples(input_duration) /
				render_speed);

			if(rqlen > packed_buffer_len)
			{
				for(int i = 0; i < MAX_CHANNELS; i++)
				{
					delete [] audio_out_packed[i];
					audio_out_packed[i] = 0;
				}
				for(int i = 0; i < audio_channels; i++)
					audio_out_packed[i] = new double[rqlen];
				packed_buffer_len = rqlen;
			}
			in_process = audio_out_packed;
		}
		for(int i = 0; i < audio_channels; i++)
		{
			int in, out;
			double *current_buffer, *orig_buffer;
			int out_length = audio_out[0]->length;

			orig_buffer = audio_buf[i] = audio_out[i]->buffer;
			current_buffer = audio_out_packed[i];

			if(render_speed > 1)
			{
				int interpolate_len = round(render_speed);
				real_output_len = out_length / interpolate_len;

				if(render_direction == PLAY_FORWARD)
				{
					for(in = 0, out = 0; in < out_length;)
					{
						sample = 0;
						for(int k = 0; k < interpolate_len; k++)
							sample += orig_buffer[in++];
						current_buffer[out++] = sample / render_speed;
					}
				}
				else
				{
					for(in = out_length - 1, out = 0; in >= 0; )
					{
						sample = 0;
						for(int k = 0; k < interpolate_len; k++)
							sample += orig_buffer[in--];
						current_buffer[out++] = sample;
					}
				}
			}
			else if(render_speed < 1)
			{
				int interpolate_len = (int)(1.0 / render_speed);

				real_output_len = out_length * interpolate_len;
				if(render_direction == PLAY_FORWARD)
				{
					for(in = 0, out = 0; in < out_length;)
					{
						for(int k = 0; k < interpolate_len; k++)
							current_buffer[out++] = orig_buffer[in];
						in++;
					}
				}
				else
				{
					for(in = out_length - 1, out = 0; in >= 0;)
					{
						for(int k = 0; k < interpolate_len; k++)
							current_buffer[out++] = orig_buffer[in];
						in--;
					}
				}
			}
			else
			{
				if(render_direction == PLAY_REVERSE)
				{
					for(int s = 0, e = out_length - 1; e > s; e--, s++)
					{
						sample = orig_buffer[s];
						orig_buffer[s] = orig_buffer[e];
						orig_buffer[e] = sample;
					}
				}
				real_output_len = out_length;
				in_process = audio_buf;
			}
		}
		renderengine->set_tracking_position(render_pts, TRACK_AUDIO);
		advance_position(input_duration);
		renderengine->audio->write_buffer(in_process, real_output_len);

		if(first_buffer)
		{
			renderengine->wait_another("VirtualAConsole::process_buffer",
				TRACK_AUDIO);
			first_buffer = 0;
		}
		if(renderengine->audio->get_interrupted())
			break;
		if(last_playback)
		{
			renderengine->audio->set_last_buffer();
			break;
		}
	}
	renderengine->audio->wait_for_completion();
	renderengine->stop_tracking(render_pts, TRACK_AUDIO);
	renderengine->render_start_lock->unlock();
}

void AudioRender::get_aframes(ptstime pts, ptstime input_duration)
{
	int input_len = audio_out[0]->to_samples(input_duration);

	for(int i = 0; i < out_channels; i++)
	{
		audio_out[i]->init_aframe(pts, input_len);
		audio_out[i]->source_duration = input_duration;
	}
	process_frames();
}

void AudioRender::process_frames()
{
	ATrackRender *trender;

	for(Track *track = edl->tracks->last; track; track = track->previous)
	{
		if(track->data_type != TRACK_AUDIO || track->renderer)
			continue;
		track->renderer = new ATrackRender(track, this);
		((ATrackRender*)track->renderer)->module_levels->reset(
			renderengine->fragment_len,
			edl->this_edlsession->sample_rate, 1);
	}

	for(Track *track = edl->tracks->last; track; track = track->previous)
	{
		if(track->data_type != TRACK_AUDIO)
			continue;
		((ATrackRender*)track->renderer)->get_aframes(audio_out, out_channels);
	}
}

int AudioRender::get_output_levels(double *levels, ptstime pts)
{
	return output_levels->get_levels(levels, pts);
}

int AudioRender::get_track_levels(double *levels, ptstime pts)
{
	int i = 0;

	for(Track *track = edl->tracks->first; track; track = track->next)
	{
		if(track->data_type != TRACK_AUDIO)
			continue;
		if(track->renderer)
		{
			((ATrackRender*)track->renderer)->module_levels->get_levels(
				&levels[i++], pts);
		}
	}
	return i;
}

AFrame *AudioRender::get_file_frame(ptstime pts, ptstime duration,
	Edit *edit, int filenum)
{
	int channels;
	int last_file;
	File *file;
	int channel = edit->channel;
	Asset *asset = edit->asset;

	for(int i = 0; i < input_frames.total; i++)
	{
		InFrame *infile = input_frames.values[i];

		if(infile->file->asset == asset && infile->aframe.channel == channel &&
			infile->filenum == filenum)
		{
			if(PTSEQU(infile->aframe.pts, pts) &&
					PTSEQU(infile->aframe.duration, duration))
				return &infile->aframe;
		}
	}

	for(int i = 0; i < input_frames.total; i++)
	{
		InFrame *infile = input_frames.values[i];

		if(infile->file->asset == asset && infile->filenum == filenum)
		{
			int channels = asset->channels;

			for(int j = 0; j < channels; j++)
			{
				AFrame *aframe = &input_frames.values[i + j]->aframe;

				aframe->check_buffer(out_length);
				aframe->samplerate = out_samplerate;
				aframe->reset_buffer();
				aframe->set_fill_request(pts, duration);
				aframe->source_pts = pts - edit->get_pts() +
					edit->get_source_pts();
				infile->file->get_samples(aframe);
			}
			return &input_frames.values[i + channel]->aframe;
		}
	}

	channels = edit->asset->channels;
	last_file = input_frames.total;
	file = new File();

	if(file->open_file(edit->asset, FILE_OPEN_READ | FILE_OPEN_AUDIO))
	{
		errormsg("AudioRender::get_file_frame:Failed to open media %s",
			asset->path);
		delete file;
		return 0;
	}

	for(int j = 0; j < channels; j++)
	{
		InFrame *infile = new InFrame(file, j, out_length, filenum);

		infile->aframe.set_fill_request(pts, duration);
		infile->aframe.channel = j;
		infile->aframe.source_pts = pts - edit->get_pts() +
			edit->get_source_pts();
		file->get_samples(&infile->aframe);
		input_frames.append(infile);
	}
	return &input_frames.values[last_file + channel]->aframe;
}

InFrame::InFrame(File *file, int channel, int out_length, int filenum)
{
	this->file = file;
	this->filenum = filenum;
	aframe.check_buffer(out_length);
	aframe.channel = channel;
}

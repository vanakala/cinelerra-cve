// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "aframe.h"
#include "asset.h"
#include "atmpframecache.h"
#include "atrackrender.h"
#include "autos.h"
#include "audiodevice.h"
#include "audiorender.h"
#include "bcsignals.h"
#include "clip.h"
#include "condition.h"
#include "edl.h"
#include "edit.h"
#include "edlsession.h"
#include "file.h"
#include "keyframe.h"
#include "mainerror.h"
#include "plugin.h"
#include "pluginclient.h"
#include "renderbase.h"
#include "renderengine.inc"
#include "track.h"
#include "tracks.h"

AudioRender::AudioRender(RenderEngine *renderengine, EDL *edl)
 : RenderBase(renderengine, edl)
{
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		audio_out[i] = 0;
		audio_out_packed[i] = 0;
	}
	packed_buffer_len = 0;
}

AudioRender::~AudioRender()
{
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		audio_frames.release_frame(audio_out[i]);
		delete [] audio_out_packed[i];
	}
	input_frames.remove_all_objects();
}

void AudioRender::init_frames()
{
	out_channels = edl->this_edlsession->audio_channels;
	out_length = renderengine->fragment_len;
	out_samplerate = edl->this_edlsession->sample_rate;

	if(renderengine->playback_engine)
	{
		for(int i = 0; i < MAXCHANNELS; i++)
		{
			if(i < edl->this_edlsession->audio_channels)
			{
				if(audio_out[i])
					audio_out[i]->check_buffer(out_length);
				else
					audio_out[i] = audio_frames.get_tmpframe(out_length);
				audio_out[i]->set_samplerate(out_samplerate);
				audio_out[i]->channel = i;
			}
		}
		sample_duration = audio_out[0]->to_duration(1);
	}
	output_levels.reset(out_length, out_samplerate, out_channels);

	for(Track *track = edl->tracks->last; track; track = track->previous)
	{
		if(track->data_type != TRACK_AUDIO || track->renderer)
			continue;
		track->renderer = new ATrackRender(track, this);
		((ATrackRender*)track->renderer)->module_levels.reset(
			renderengine->fragment_len,
			edl->this_edlsession->sample_rate, 1);
	}
}

int AudioRender::process_buffer(AFrame **buffer_out)
{
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		if(buffer_out[i])
		{
			AFrame *cur_buf = buffer_out[i];

			audio_out[i] = cur_buf;
			audio_out[i]->clear_frame(cur_buf->get_pts(),
				cur_buf->get_source_duration());
		}
		else
			audio_out[i] = 0;
	}
	sample_duration = audio_out[0]->to_duration(1);
	process_frames();
	// Not ours, must not delete them
	for(int i = 0; i < MAXCHANNELS; i++)
		audio_out[i] = 0;
	return 0;
}

ptstime AudioRender::calculate_render_duration()
{
	Autos *autos;
	Auto *autom;
	Edit *edit;
	ArrayList<Plugin*> *plugins;
	KeyFrame *keyframe;
	ptstime render_duration = audio_out[0]->to_duration(renderengine->fragment_len);
	ptstime start_pts = render_pts;
	ptstime end_pts;

	if(render_direction == PLAY_FORWARD)
	{
		end_pts = start_pts + render_duration;
		if(end_pts >= render_end)
			end_pts = render_end;
		// start_pts .. end_pts
		for(Track *track = edl->tracks->first; track; track = track->next)
		{
			if(!track->renderer || track->data_type != TRACK_AUDIO)
				continue;
			if(edit = track->renderer->media_track->editof(start_pts))
			{
				if(edit->end_pts() < end_pts && edit->end_pts() > start_pts)
					end_pts = edit->end_pts();
			}
			for(int i = 0; i < AUTOMATION_TOTAL; i++)
			{
				if(!(autos = track->renderer->autos_track->automation->have_autos(i)))
					continue;
				if(!(autom = autos->nearest_after(start_pts)))
					continue;
				if(autom->pos_time < end_pts && autom->pos_time > start_pts)
					end_pts = autom->pos_time;
			}
			plugins = &track->renderer->plugins_track->plugins;
			for(int i = 0; i < plugins->total; i++)
			{
				Plugin *plugin = plugins->values[i];

				if(plugin->active_in(start_pts, end_pts))
				{
					// Plugin starts in frame
					if(plugin->get_pts() > start_pts)
						end_pts = plugin->get_pts() - track->one_unit;
					// Plugin ends in frame
					else if(plugin->end_pts() < end_pts)
						end_pts = plugin->end_pts();
					// Check keyframes
					if(keyframe = plugin->get_next_keyframe(start_pts))
					{
						if(keyframe->pos_time > start_pts &&
								keyframe->pos_time < end_pts)
							end_pts = keyframe->pos_time;
					}
				}
			}
		}
	}
	else
	{
		end_pts = start_pts - render_duration;
		// end_pts .. start_pts
		if(end_pts <= render_start)
			end_pts = render_start;

		for(Track *track = edl->tracks->first; track; track = track->next)
		{
			if(!track->renderer || track->data_type != TRACK_AUDIO)
				continue;
			if(edit = track->renderer->media_track->editof(start_pts))
			{
				if(edit->get_pts() > end_pts && edit->get_pts() < start_pts)
					end_pts = edit->get_pts();
			}
			for(int i = 0; i < AUTOMATION_TOTAL; i++)
			{
				if(!(autos = track->renderer->autos_track->automation->have_autos(i)))
					continue;
				if(!(autom = autos->nearest_before(start_pts)))
					continue;
				if(autom->pos_time > end_pts && autom->pos_time < start_pts)
					end_pts = autom->pos_time;
			}

			plugins = &track->renderer->plugins_track->plugins;
			for(int i = 0; i < plugins->total; i++)
			{
				Plugin *plugin = plugins->values[i];

				if(plugin->active_in(end_pts, start_pts))
				{
					// Plugin starts in frame
					if(plugin->get_pts() > end_pts)
						end_pts = plugin->get_pts() - track->one_unit;
					// Plugin ends in frame
					else if(plugin->end_pts() > end_pts)
						end_pts = plugin->end_pts();
					// Check keyframes
					if(keyframe = plugin->get_prev_keyframe(start_pts))
					{
						if(keyframe->pos_time > end_pts &&
								keyframe->pos_time < start_pts)
							end_pts = keyframe->pos_time;
					}
				}
			}
		}
	}
	return fabs(end_pts - start_pts);
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
		input_duration = calculate_render_duration();

		if(input_duration > sample_duration)
		{
			get_aframes(render_pts, input_duration);

			output_levels.fill(audio_out);

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
				int out_length = audio_out[0]->get_length();

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
			renderengine->audio->write_buffer(in_process, real_output_len);
		}
		else
			input_duration = sample_duration;

		advance_position(input_duration);

		if(first_buffer)
		{
			renderengine->wait_another("AudioRender::run",
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
	renderengine->stop_tracking(-1, TRACK_AUDIO);
	renderengine->render_start_lock->unlock();
}

void AudioRender::get_aframes(ptstime pts, ptstime input_duration)
{
	int input_len = audio_out[0]->to_samples(input_duration);

	for(int i = 0; i < out_channels; i++)
	{
		audio_out[i]->init_aframe(pts, input_len, audio_out[0]->get_samplerate());
		audio_out[i]->clear_frame(pts, input_duration);
		audio_out[i]->set_source_duration(input_duration);
	}
	process_frames();
}

void AudioRender::process_frames()
{
	ATrackRender *trender;
	int found;
	int count = 0;

	for(Track *track = edl->tracks->last; track; track = track->previous)
	{
		if(track->data_type != TRACK_AUDIO)
			continue;
		((ATrackRender*)track->renderer)->process_aframes(audio_out,
				out_channels, RSTEP_NORMAL);
	}

	for(;;)
	{
		found = 0;
		for(Track *track = edl->tracks->last; track; track = track->previous)
		{
			if(track->data_type != TRACK_AUDIO || track->renderer->track_ready())
				continue;
			((ATrackRender*)track->renderer)->process_aframes(audio_out,
					out_channels, RSTEP_SHARED);
			found = 1;
		}
		if(!found)
			break;
		if(++count > 3)
			break;
	}

	for(Track *track = edl->tracks->last; track; track = track->previous)
	{
		if(track->data_type != TRACK_AUDIO)
			continue;
		((ATrackRender*)track->renderer)->render_pan(audio_out, out_channels);
		track->renderer->next_plugin = 0;
	}
}

int AudioRender::get_output_levels(double *levels, ptstime pts)
{
	return output_levels.get_levels(levels, pts);
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
			((ATrackRender*)track->renderer)->module_levels.get_levels(
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
	int channel, stream;
	Asset *asset;
	AFrame *cur;

	if(!edit)
		return 0;

	channel = edit->channel;
	stream = edit->stream;
	asset = edit->asset;

	if(!asset)
	{
		cur = audio_frames.get_tmpframe(out_length);
		cur->set_samplerate(out_samplerate);
		cur->channel = channel;
		cur->clear_frame(pts, duration);
		return cur;
	}

	for(int i = 0; i < input_frames.total; i++)
	{
		InFrame *infile = input_frames.values[i];

		if(infile->file->asset == asset && infile->channel == channel &&
			infile->stream == stream && infile->filenum == filenum)
		{
			cur = infile->get_aframe(stream, channel);

			if(PTSEQU(cur->get_pts(), pts) &&
					PTSEQU(cur->get_duration(), duration))
				return infile->handover_aframe();
		}
	}

	for(int i = 0; i < input_frames.total; i++)
	{
		InFrame *infile = input_frames.values[i];

		if(infile->file->asset == asset && infile->filenum == filenum &&
			infile->stream == stream)
		{
			int channels = asset->streams[stream].channels;

			for(int j = 0; j < channels; j++)
			{
				AFrame *aframe = input_frames.values[i + j]->get_aframe(stream, j);

				aframe->set_samplerate(out_samplerate);
				aframe->reset_buffer();
				aframe->set_fill_request(pts, duration);
				aframe->set_source_pts(pts - edit->get_pts() +
					edit->get_source_pts() - edit->track->nudge);
				infile->file->get_samples(aframe);
			}
			return input_frames.values[i + channel]->handover_aframe();
		}
	}

	channels = edit->asset->streams[stream].channels;
	last_file = input_frames.total;
	file = new File();

	if(file->open_file(asset, FILE_OPEN_READ | FILE_OPEN_AUDIO, stream))
	{
		errormsg("AudioRender::get_file_frame:Failed to open media %s",
			asset->path);
		delete file;
		return 0;
	}

	for(int j = 0; j < channels; j++)
	{
		InFrame *infile = new InFrame(file, out_length, filenum);

		cur = infile->get_aframe(stream, j);
		cur->set_samplerate(out_samplerate);
		cur->set_fill_request(pts, duration);
		cur->set_source_pts(pts - edit->get_pts() +
			edit->get_source_pts() - edit->track->nudge);
		file->get_samples(cur);
		input_frames.append(infile);
	}
	return input_frames.values[last_file + channel]->handover_aframe();
}

void AudioRender::pass_aframes(Plugin *plugin, AFrame *current_frame,
	ATrackRender *current_renderer, Edit *edit)
{
	AFrame *aframe;
	ptstime pts = current_frame->get_pts();

	current_renderer->aframes.remove_all();
	current_renderer->aframes.append(current_frame);

	// Add frames for other tracks starting from the first
	for(Track *track = edl->tracks->first; track; track = track->next)
	{
		if(track->data_type != TRACK_AUDIO ||
				!track->renderer->is_playable(pts, edit))
			continue;
		for(int i = 0; i < track->plugins.total; i++)
		{
			if(track->plugins.values[i]->shared_plugin == plugin &&
				track->plugins.values[i]->on)
			{
				if(aframe = ((ATrackRender*)track->renderer)->handover_trackframe())
					current_renderer->aframes.append(aframe);
			}
		}
	}

	plugin->client->plugin_init(current_renderer->aframes.total);
}

AFrame *AudioRender::take_aframes(Plugin *plugin, ATrackRender *current_renderer,
	Edit *edit)
{
	int k = 1;
	AFrame *current_frame = current_renderer->aframes.values[0];
	ptstime pts = current_frame->get_pts();

	for(Track *track = edl->tracks->first; track; track = track->next)
	{
		if(track->data_type != TRACK_AUDIO ||
				!track->renderer->is_playable(pts, edit))
			continue;
		for(int i = 0; i < track->plugins.total; i++)
		{
			if(track->plugins.values[i]->shared_plugin == plugin)
			{
				if(k >= current_renderer->aframes.total)
					break;
				AFrame *aframe = current_renderer->aframes.values[k];
				if(aframe->get_track() == track->number_of())
				{
					((ATrackRender*)track->renderer)->take_aframe(aframe);
					k++;
				}
			}
		}
	}
	return current_frame;
}

void AudioRender::release_asset(Asset *asset)
{
	File *files[MAXCHANNELS];
	int last_file = 0;
	int j;

	for(int i = 0; i < input_frames.total; i++)
	{
		InFrame *infile = input_frames.values[i];

		if(infile->file->asset == asset)
		{
			for(j = 0; j < last_file; j++)
			{
				if(files[j] == infile->file)
					break;
			}
			if(j == last_file)
				files[last_file++] = infile->file;
			input_frames.remove_number(i);
			i--;
			delete infile;
		}
	}

	for(int i = 0; i < last_file; i++)
		delete files[i];
}

InFrame::InFrame(File *file, int out_length, int filenum)
{
	this->file = file;
	this->filenum = filenum;
	this->out_length = out_length;
	channel = -1;
	stream = -1;
	aframe = 0;
}

InFrame::~InFrame()
{
	audio_frames.release_frame(aframe);
}

AFrame *InFrame::get_aframe(int strm, int chnl)
{
	if(!aframe)
	{
		aframe = audio_frames.get_tmpframe(out_length);
		aframe->channel = channel = chnl;
		stream = strm;
		aframe->reset_pts();
	}
	return aframe;
}

AFrame *InFrame::handover_aframe()
{
	AFrame *cur = aframe;

	aframe = 0;
	channel = -1;
	return cur;
}

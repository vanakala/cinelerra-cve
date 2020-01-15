
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
#include "atmpframecache.h"
#include "asset.h"
#include "atrackrender.h"
#include "audiorender.h"
#include "automation.h"
#include "bcsignals.h"
#include "clip.h"
#include "edit.h"
#include "file.h"
#include "floatautos.h"
#include "panauto.h"
#include "panautos.h"
#include "plugin.h"
#include "pluginserver.h"
#include "track.h"
#include "units.h"

ATrackRender::ATrackRender(Track *track, AudioRender *audiorender)
 : TrackRender(track)
{
	arender = audiorender;
	track_frame = 0;
}

ATrackRender::~ATrackRender()
{
	audio_frames.release_frame(track_frame);
}

void ATrackRender::process_aframes(AFrame **output, int out_channels, int rstep)
{
	AFrame *aframe;
	ptstime pts = output[0]->pts;
	Edit *edit = media_track->editof(pts);

	if(is_playable(pts, edit))
	{
		if(rstep == RSTEP_NORMAL)
		{
			if(aframe = arender->get_file_frame(output[0]->pts,
				output[0]->source_duration, edit, 0))
			{
				render_fade(aframe);
				render_transition(aframe, edit);
				render_plugins(aframe, edit, rstep);
			}
		}
		else
		{
			if(next_plugin)
			{
				render_plugins(track_frame, edit, rstep);
				aframe = track_frame;
			}
			else
				return;
		}
		if(!next_plugin)
		{
			module_levels.fill(&aframe);
			track_frame = aframe;
		}
	}
}

AFrame *ATrackRender::get_aframe(AFrame *buffer)
{
// Called by plugin
	AFrame *aframe;
	ptstime buffer_pts = buffer->pts;
	ptstime buffer_duration = buffer->source_duration;
	int channel = buffer->channel;

	if(track_frame && track_frame->channel == channel &&
			PTSEQU(track_frame->pts, buffer_pts))
		buffer->copy(track_frame);
	else
	{
		Edit *edit = media_track->editof(buffer_pts);

		if(aframe = arender->get_file_frame(buffer->pts,
			buffer->source_duration, edit, 2))
		{
			buffer->copy(aframe);
			render_fade(buffer);
			render_transition(buffer, edit);
			audio_frames.release_frame(aframe);
		}
		else
			buffer->clear_frame(buffer->pts, buffer->source_duration);
	}
	return buffer;
}

void ATrackRender::render_pan(AFrame **output, int out_channels)
{
	double intercept;
	double slope = 0;

	if(!track_frame)
		return;

	if(track_frame->channel >= 0)
	{
		PanAutos *panautos = (PanAutos*)autos_track->automation->autos[AUTOMATION_PAN];
		ptstime pts = track_frame->pts;

		for(int i = 0; i < out_channels; i++)
		{
			int slope_length = track_frame->length;
			ptstime slope_step = track_frame->to_duration(1);
			ptstime val = 0;

			if(!panautos->first)
				intercept = panautos->default_values[i];
			else
			{
				PanAuto *prev = (PanAuto*)panautos->get_prev_auto(track_frame->pts);
				PanAuto *next = (PanAuto*)panautos->get_next_auto(track_frame->pts, prev);

				if(prev && next && prev != next)
				{
					slope = (next->values[i] - prev->values[i]) /
						(next->pos_time - prev->pos_time);
					intercept = (pts - prev->pos_time) * slope +
						prev->values[i];
				}
				else
					intercept = prev->values[i];
			}

			if(!EQUIV(slope, 0))
			{
				for(int j = 0; j < slope_length; j++)
				{
					output[i]->buffer[j] += track_frame->buffer[j] *
						(slope * val + intercept);
					val += slope_step;
				}
			}
			else if(!EQUIV(intercept, 0))
			{
				for(int j = 0; j < slope_length; j++)
					output[i]->buffer[j] += track_frame->buffer[j] * intercept;
			}
		}
	}
	audio_frames.release_frame(track_frame);
	track_frame = 0;
}

void ATrackRender::render_fade(AFrame *aframe)
{
	FloatAutos *fadeautos = (FloatAutos*)autos_track->automation->autos[AUTOMATION_FADE];
	ptstime pts = aframe->pts;
	int framelen = aframe->length;
	double fade_value, value;

	if(fadeautos->automation_is_constant(pts, aframe->duration, fade_value))
	{
		if(EQUIV(fade_value, 0))
			return;
		if(fade_value <= INFINITYGAIN)
			value = 0;
		else
			value = DB::fromdb(fade_value);

		for(int i = 0; i < framelen; i++)
			aframe->buffer[i] *= value;
	}
	else
	{
		double step = 1.0 / aframe->samplerate;

		for(int i = 0; i < framelen; i++)
		{
			fade_value = fadeautos->get_value(pts);
			pts += step;

			if(fade_value <= INFINITYGAIN)
				value = 0;
			else
				value = DB::fromdb(fade_value);
			aframe->buffer[i] *= value;
		}
	}
}

void ATrackRender::render_transition(AFrame *aframe, Edit *edit)
{
	Plugin *transition = edit->transition;
	AFrame *tmpframe;
	Edit *prev = edit->previous;

	if(!transition || !transition->plugin_server || !transition->on ||
			transition->get_length() < aframe->pts - edit->get_pts())
		return;

	if(!transition->active_server)
	{
		transition->active_server = new PluginServer(*transition->plugin_server);
		transition->active_server->open_plugin(0, transition, this);
		transition->active_server->init_realtime(1);
	}

	if(!(tmpframe = arender->get_file_frame(aframe->pts,
		aframe->source_duration, prev, 1)))
	{
		tmpframe = audio_frames.clone_frame(aframe);
		tmpframe->clear_frame(aframe->pts, aframe->source_duration);
	}

	transition->active_server->process_transition(tmpframe, aframe,
		aframe->pts - edit->get_pts(), transition->get_length());
	audio_frames.release_frame(tmpframe);
}

void ATrackRender::render_plugins(AFrame *aframe, Edit *edit, int rstep)
{
	Plugin *plugin;
	AFrame *tmp;
	ptstime start = aframe->pts;
	ptstime end = start + aframe->duration;

	track_frame = aframe;
	current_edit = edit;
	for(int i = 0; i < plugins_track->plugins.total; i++)
	{
		plugin = plugins_track->plugins.values[i];
		if(next_plugin)
		{
			if(next_plugin != plugin)
				continue;
			next_plugin = 0;
		}

		if(plugin->on && plugin->active_in(start, end))
		{
			track_frame->set_track(media_track->number_of());
			if(tmp = execute_plugin(plugin, track_frame, rstep))
				track_frame = tmp;
			else
			{
				track_frame = aframe;
				next_plugin = plugin;
				break;
			}
		}
	}
}

AFrame *ATrackRender::execute_plugin(Plugin *plugin, AFrame *aframe, int rstep)
{
	PluginServer *server = plugin->plugin_server;

	switch(plugin->plugin_type)
	{
	case PLUGIN_NONE:
		break;

	case PLUGIN_SHAREDMODULE:
		if(!plugin->shared_track)
			break;
		set_effects_track(plugin->shared_track);
		render_fade(aframe);
		render_transition(aframe, current_edit);
		render_plugins(aframe, current_edit, rstep);
		set_effects_track(media_track);
		break;

	case PLUGIN_SHAREDPLUGIN:
		if(!server && plugin->shared_plugin)
		{
			if(!plugin->shared_plugin->plugin_server->multichannel)
				server = plugin->shared_plugin->plugin_server;
			else if(rstep != RSTEP_NORMAL)
				return 0;
		}
		// Fall through
	case PLUGIN_STANDALONE:
		if(server)
		{
			if(server->multichannel && !plugin->shared_plugin)
			{
				if(rstep == RSTEP_NORMAL)
					return 0;
				if(!plugin->active_server)
				{
					plugin->active_server = new PluginServer(*server);
					plugin->active_server->open_plugin(0, plugin, this);
				}
				arender->allocate_aframes(plugin);

				for(int i = 0; i < plugin->aframes.total; i++)
				{
					AFrame *current = plugin->aframes.values[i];
					current->set_fill_request(aframe->pts, aframe->duration);
					current->copy_pts(aframe);
					if(i > 0)
					{
						Edit *edit = get_track_number(current->get_track())->editof(aframe->pts);
						if(edit)
							current->channel = edit->channel;
					}
				}
				plugin->active_server->process_buffer(plugin->aframes.values,
					plugin->get_length());
				aframe->copy(plugin->aframes.values[0]);
				arender->copy_aframes(&plugin->aframes, this);
			}
			else
			{
				if(!plugin->active_server)
				{
					plugin->active_server = new PluginServer(*server);
					plugin->active_server->open_plugin(0, plugin, this);
					plugin->active_server->init_realtime(1);
				}
				plugin->active_server->process_buffer(&aframe, plugin->get_length());
			}
		}
		break;
	}
	return aframe;
}

void ATrackRender::copy_track_aframe(AFrame *aframe)
{
	if(!track_frame)
		track_frame = audio_frames.clone_frame(aframe);
	track_frame->copy(aframe);
}


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
	current_aframe = 0;
}

ATrackRender::~ATrackRender()
{
	delete track_frame;
}

AFrame *ATrackRender::read_aframe(AFrame *aframe, Edit *edit, int filenum)
{
	ptstime sample_error = 0.8 / aframe->samplerate;
	AFrame *result;

	if(!(result = arender->get_file_frame(aframe->pts, aframe->source_duration,
		edit, filenum)))
	{
		if(!track_frame)
			track_frame = new AFrame(aframe->buffer_length);
		else
			track_frame->check_buffer(aframe->buffer_length);
		track_frame->copy_pts(aframe);
		track_frame->clear_frame(aframe->pts, aframe->source_duration);
		return 0;
	}

	return result;
}

AFrame **ATrackRender::get_aframes(AFrame **output, int out_channels)
{
	AFrame *aframe;
	ptstime pts = output[0]->pts;
	Edit *edit = media_track->editof(pts);

	if(is_playable(pts, edit))
	{
		aframe = read_aframe(output[0], edit, 0);
		if(!aframe)
			aframe = track_frame;
		render_fade(aframe);
		render_transition(aframe, edit);
		render_plugins(aframe, edit);
		module_levels.fill(&aframe);
		render_pan(output, out_channels, aframe);
	}
	return output;
}

AFrame *ATrackRender::get_aframe(AFrame *buffer)
{
// Called by plugin
	ptstime buffer_pts = buffer->pts;
	ptstime buffer_duration = buffer->source_duration;
	int channel = buffer->channel;

	if(current_aframe && current_aframe->channel == channel &&
			PTSEQU(current_aframe->pts, buffer_pts))
		buffer->copy(current_aframe);
	else
	{
		Edit *edit = media_track->editof(buffer_pts);

		AFrame *aframe = read_aframe(buffer, edit, 2);
		buffer->copy(aframe);
	}
	return buffer;
}

void ATrackRender::render_pan(AFrame **output, int out_channels,
	AFrame *aframe)
{
	double intercept;
	double slope = 0;
	PanAutos *panautos = (PanAutos*)autos_track->automation->autos[AUTOMATION_PAN];
	ptstime pts = aframe->pts;

	for(int i = 0; i < out_channels; i++)
	{
		int slope_length = aframe->length;
		ptstime slope_step = aframe->to_duration(1);
		ptstime val = 0;

		if(!panautos->first)
			intercept = panautos->default_values[i];
		else
		{
			PanAuto *prev = (PanAuto*)panautos->get_prev_auto(aframe->pts);
			PanAuto *next = (PanAuto*)panautos->get_next_auto(aframe->pts, prev);

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
				output[i]->buffer[j] += aframe->buffer[j] *
					(slope * val + intercept);
				val += slope_step;
			}
		}
		else if(!EQUIV(intercept, 0))
		{
			for(int j = 0; j < slope_length; j++)
				output[i]->buffer[j] += aframe->buffer[j] * intercept;
		}
	}
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

	if(!(tmpframe = read_aframe(aframe, prev, 1)))
		tmpframe = track_frame;

	transition->active_server->process_transition(tmpframe, aframe,
		aframe->pts - edit->get_pts(), transition->get_length());
}

void ATrackRender::render_plugins(AFrame *aframe, Edit *edit)
{
	Plugin *plugin;
	ptstime start = aframe->pts;
	ptstime end = start + aframe->duration;
	AFrame *tmpframe;

	current_aframe = aframe;
	current_edit = edit;

	for(int i = 0; i < plugins_track->plugins.total; i++)
	{
		plugin = plugins_track->plugins.values[i];

		if(plugin->on && plugin->active_in(start, end))
		{
			current_aframe->set_track(media_track->number_of());
			if(tmpframe = execute_plugin(plugin, current_aframe))
				current_aframe = tmpframe;
		}
	}
	current_aframe = 0;
}

AFrame *ATrackRender::execute_plugin(Plugin *plugin, AFrame *aframe)
{
	PluginServer *server = plugin->plugin_server;

	switch(plugin->plugin_type)
	{
	case PLUGIN_NONE:
		break;

	case PLUGIN_SHAREDMODULE:
		// not ready
		break;

	case PLUGIN_SHAREDPLUGIN:
		if(!server && plugin->shared_plugin)
		{
			if(!plugin->shared_plugin->plugin_server->multichannel)
				server = plugin->shared_plugin->plugin_server;
		}
		// Fall through
	case PLUGIN_STANDALONE:
		if(server)
		{
			if(server->multichannel && !plugin->shared_plugin)
			{
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
				return aframe;
			}
		}
		break;
	}
	return 0;
}

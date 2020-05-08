
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
#include "pluginclient.h"
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
	ptstime pts = output[0]->get_pts();
	Edit *edit = media_track->editof(pts);

	if(is_playable(pts, edit))
	{
		if(rstep == RSTEP_NORMAL)
		{
			if(aframe = arender->get_file_frame(output[0]->get_pts(),
				output[0]->get_source_duration(), edit, 0))
			{
				render_fade(aframe);
				render_transition(aframe, edit);
				track_frame = render_plugins(aframe, edit, rstep);
			}
		}
		else
		{
			if(next_plugin)
				track_frame = render_plugins(track_frame, edit, rstep);
			else
				return;
		}
		if(!next_plugin)
			module_levels.fill(&track_frame);
	}
	else
		next_plugin = 0;
}

AFrame *ATrackRender::get_aframe(AFrame *buffer)
{
// Called by plugin
	AFrame *aframe;
	ptstime buffer_pts = buffer->get_pts();
	ptstime buffer_duration = buffer->get_source_duration();
	int channel = buffer->channel;

	if(track_frame && track_frame->channel == channel &&
			PTSEQU(track_frame->get_pts(), buffer_pts))
		buffer->copy(track_frame);
	else
	{
		Edit *edit = media_track->editof(buffer_pts);

		if(aframe = arender->get_file_frame(buffer->get_pts(),
			buffer->get_source_duration(), edit, 2))
		{
			buffer->copy(aframe);
			render_fade(buffer);
			render_transition(buffer, edit);
			audio_frames.release_frame(aframe);
		}
		else
			buffer->clear_frame(buffer->get_pts(),
				buffer->get_source_duration());
	}
	return buffer;
}

AFrame *ATrackRender::get_atmpframe(AFrame *buffer, PluginClient *client)
{
// Called by tmpframe aware plugin
	AFrame *aframe;
	ptstime buffer_pts = buffer->get_pts();
	Plugin *current = client->plugin;
	Edit *edit = media_track->editof(buffer_pts);
	ptstime plugin_start = current->get_pts();

	if(buffer_pts > current->end_pts() ||
			buffer_pts - buffer->get_source_duration() < plugin_start)
		edit = 0;
	else if(buffer_pts < plugin_start)
		buffer->set_fill_request(buffer_pts,
			buffer->get_source_duration() - buffer_pts + plugin_start);

	if(edit && (aframe = arender->get_file_frame(buffer_pts,
			buffer->get_source_duration(), edit, 2)))
	{
		render_fade(aframe);
		render_transition(aframe, edit);
		// render all standalone plugns before the current
		for(int i = 0; i < plugins_track->plugins.total; i++)
		{
			Plugin *plugin = plugins_track->plugins.values[i];

			if(plugin == current)
				break;
			if(!plugin->plugin_server)
				continue;
			if(plugin->plugin_type != PLUGIN_STANDALONE ||
					plugin->plugin_server->multichannel)
				continue;
			aframe = execute_plugin(plugin, aframe, RSTEP_NORMAL);
		}
		audio_frames.release_frame(buffer);
		buffer = aframe;
	}
	else
		buffer->clear_frame(buffer->get_pts(), buffer->get_source_duration());
	return buffer;
}

void ATrackRender::render_pan(AFrame **output, int out_channels)
{
	double intercept;

	if(!track_frame)
		return;

	if(track_frame->channel >= 0)
	{
		PanAutos *panautos = (PanAutos*)autos_track->automation->autos[AUTOMATION_PAN];
		ptstime pts = track_frame->get_pts();

		for(int i = 0; i < out_channels; i++)
		{
			int slope_length = track_frame->get_length();
			ptstime slope_step = track_frame->to_duration(1);
			ptstime val = 0;
			double slope = 0;

			if(!panautos->first)
				intercept = panautos->default_values[i];
			else
			{
				PanAuto *prev = (PanAuto*)panautos->get_prev_auto(track_frame->get_pts());
				PanAuto *next = (PanAuto*)panautos->get_next_auto(track_frame->get_pts(), prev);

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
	ptstime pts = aframe->get_pts();
	int framelen = aframe->get_length();
	double fade_value, value;

	if(fadeautos->automation_is_constant(pts, aframe->get_duration(), fade_value))
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
		double step = aframe->to_duration(1);

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
			transition->get_length() < aframe->get_pts() - edit->get_pts())
		return;

	if(!transition->client)
	{
		transition->plugin_server->open_plugin(transition, this);
		transition->client->plugin_init(1);
	}

	if(!(tmpframe = arender->get_file_frame(aframe->get_pts(),
		aframe->get_source_duration(), prev, 1)))
	{
		tmpframe = audio_frames.clone_frame(aframe);
		tmpframe->clear_frame(aframe->get_pts(), aframe->get_source_duration());
	}

	transition->client->process_transition(tmpframe, aframe,
		aframe->get_pts() - edit->get_pts(), transition->get_length());
	audio_frames.release_frame(tmpframe);
}

AFrame *ATrackRender::render_plugins(AFrame *aframe, Edit *edit, int rstep)
{
	Plugin *plugin;
	AFrame *tmp;
	ptstime start = aframe->get_pts();
	ptstime end = start + aframe->get_duration();

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
			aframe->set_track(media_track->number_of());
			if(tmp = execute_plugin(plugin, aframe, rstep))
				aframe = tmp;
			else
			{
				next_plugin = plugin;
				break;
			}
		}
	}
	return aframe;
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
		aframe = render_plugins(aframe, current_edit, rstep);
		set_effects_track(media_track);
		break;

	case PLUGIN_SHAREDPLUGIN:
		if(!server && plugin->shared_plugin)
		{
			if(!plugin->shared_plugin->plugin_server->multichannel)
				server = plugin->shared_plugin->plugin_server;
			else
			{
				if(rstep == RSTEP_NORMAL)
					return 0;
			}
		}
		// Fall through
	case PLUGIN_STANDALONE:
		if(server)
		{
			if(server->multichannel && !plugin->shared_plugin)
			{
				if(!arender->is_shared_ready(plugin, aframe->get_pts()))
					return 0;
				if(!plugin->client)
					plugin->plugin_server->open_plugin(plugin, this);
				plugin->client->set_renderer(this);

				if(plugin->apiversion < 3)
				{
					arender->allocate_aframes(plugin);

					for(int i = 0; i < plugin->aframes.total; i++)
					{
						AFrame *current = plugin->aframes.values[i];
						current->set_fill_request(aframe->get_pts(),
							aframe->get_duration());
						current->copy_pts(aframe);
						if(i > 0)
						{
							Track *pltrack = get_track_number(current->get_track());
							Edit *edit = pltrack->editof(aframe->get_pts());
							if(edit)
								current->channel = edit->channel;
							pltrack->renderer->next_plugin = 0;
						}
					}
					plugin->client->process_buffer(plugin->aframes.values);
					aframe->copy(plugin->aframes.values[0]);
					arender->copy_aframes(&plugin->aframes, this);
				}
				else
				{
					arender->pass_aframes(plugin, this);
					plugin->client->process_buffer(aframes.values);
					arender->take_aframes(plugin, this);
				}
				next_plugin = 0;
			}
			else
			{
				if(!plugin->client)
				{
					plugin->plugin_server->open_plugin(plugin, this);
					plugin->client->plugin_init(1);
				}
				plugin->client->set_renderer(this);
				plugin->client->process_buffer(&aframe);
			}
		}
		break;
	}
	return aframe;
}

void ATrackRender::copy_track_aframe(AFrame *aframe)
{
	if(!is_muted(aframe->get_pts()))
	{
		if(!track_frame)
			track_frame = audio_frames.clone_frame(aframe);
		track_frame->copy(aframe);
	}
}

AFrame *ATrackRender::handover_trackframe()
{
	AFrame *tmp = track_frame;

	track_frame = 0;
	return tmp;
}

void ATrackRender::take_aframe(AFrame *frame)
{
	track_frame = frame;
}

void ATrackRender::dump(int indent)
{
	printf("%*sATrackRender %p dump:\n", indent, "", this);
	indent += 2;
	printf("%*strack_frame %p current_edit %p audiorender %p\n", indent, "",
		track_frame, current_edit, arender);
	TrackRender::dump(indent);
}

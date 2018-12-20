
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

#include "aattachmentpoint.h"
#include "aframe.h"
#include "amodule.h"
#include "arender.h"
#include "asset.h"
#include "atrack.h"
#include "bcsignals.h"
#include "cache.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filexml.h"
#include "floatautos.h"
#include "language.h"
#include "levelhist.h"
#include "module.h"
#include "mainerror.h"
#include "plugin.h"
#include "pluginarray.h"
#include "preferences.h"
#include "renderengine.h"
#include "mainsession.h"
#include "sharedlocation.h"
#include "theme.h"
#include "transition.h"

#include <string.h>

AModule::AModule(RenderEngine *renderengine, 
	CommonRender *commonrender, 
	PluginArray *plugin_array,
	Track *track)
 : Module(renderengine, commonrender, plugin_array, track)
{
	data_type = TRACK_AUDIO;
	module_levels = 0;
	transition_temp = 0;
	if(commonrender)
	{
		module_levels = new LevelHistory();
		module_levels->reset(get_buffer_size(),
			edlsession->sample_rate, 1);
	}
}

AModule::~AModule()
{
	if(transition_temp) delete transition_temp;
	if(module_levels) delete module_levels;
}

AttachmentPoint* AModule::new_attachment(Plugin *plugin)
{
	return new AAttachmentPoint(renderengine, plugin);
}

void AModule::reset()
{
	Module::reset();
// Not needed in pluginarray
	if(commonrender)
		module_levels->reset(get_buffer_size(),
			edlsession->sample_rate, 1);
}

int AModule::get_buffer_size()
{
	if(renderengine)
		return renderengine->fragment_len;
	else
		return plugin_array->get_bufsize();
}

CICache* AModule::get_cache()
{
	if(renderengine)
		return renderengine->get_acache();
	else
		return cache;
}

int AModule::render(AFrame *aframe)
{
	Edit *playable_edit;
	ptstime start_projpts = aframe->pts;
	int result = 0;

// Clear buffer
	aframe->length = 0;
	aframe->clear_buffer();
	aframe->samplerate = edlsession->sample_rate;
	ptstime duration = aframe->get_source_duration();
	ptstime end_projpts = start_projpts + duration;
	ptstime sample_error = 0.8 / aframe->samplerate;

// Get first edit containing the range
	for(playable_edit = track->edits->first;
		playable_edit;
		playable_edit = playable_edit->next)
	{
		if(start_projpts < playable_edit->end_pts() && 
				end_projpts > playable_edit->get_pts())
			break;
	}

// Fill output one fragment at a time
	while(start_projpts < end_projpts - sample_error)
	{
		ptstime fragment_duration = duration;

		if(fragment_duration + start_projpts > end_projpts)
			fragment_duration = end_projpts - start_projpts;

// Normalize position here since update_transition is a boolean operation.
		update_transition(start_projpts);

		if(playable_edit)
		{
// Normalize EDL positions to requested rate
			ptstime edit_start = playable_edit->get_pts();
			ptstime edit_end = playable_edit->end_pts();
			ptstime edit_src = playable_edit->get_source_pts();
// Trim fragment_len
			if(fragment_duration + start_projpts > edit_end)
				fragment_duration = edit_end - start_projpts;

			if(playable_edit->asset)
			{
				File *source;
				get_cache()->age();

				if(!(source = get_cache()->check_out(playable_edit->asset,
					get_edl())))
				{
// couldn't open source file / skip the edit
					result = 1;
				}
				else
				{
					aframe->source_length = 0;
					aframe->source_duration = fragment_duration;
					aframe->channel = playable_edit->channel;
					aframe->source_pts = start_projpts - edit_start + edit_src;
					source->get_samples(aframe);
					get_cache()->check_in(playable_edit->asset);
				}
			}

// Read transition into temp and render
			Edit *previous_edit = playable_edit->previous;
			if(transition && previous_edit)
			{
				ptstime transition_length = transition->length();
				ptstime previous_start = previous_edit->get_pts();
				ptstime previous_src = previous_edit->get_source_pts();

// Read into temp buffers
// Temp + master or temp + temp ? temp + master
				if(!transition_temp)
					transition_temp = new AFrame(aframe->buffer_length);
				else
					transition_temp->check_buffer(aframe->buffer_length);

				transition_temp->reset_buffer();
				transition_temp->samplerate = aframe->samplerate;

// Trim transition_len
				ptstime transition_fragment = fragment_duration;

				if(fragment_duration + start_projpts > 
						edit_start + transition_length)
					transition_fragment = edit_start + transition_length - start_projpts;

				if(transition_fragment > 0)
				{
					if(previous_edit->asset)
					{
						File *source;
						get_cache()->age();
						if(!(source = get_cache()->check_out(
							previous_edit->asset,
							get_edl())))
						{
// couldn't open source file / skip the edit
							result = 1;
						}
						else
						{
							transition_temp->source_pts = start_projpts -
									previous_start +
									previous_src;
							transition_temp->pts = start_projpts;
							transition_temp->channel = previous_edit->channel;
							transition_temp->source_duration = transition_fragment;
							transition_temp->source_length = 0;
							source->get_samples(transition_temp);
							get_cache()->check_in(previous_edit->asset);
						}
					}
					else
					{
						// Silence
						transition_temp->clear_buffer();
						transition_temp->duration = transition_fragment;
					}
					transition_server->process_transition(
						transition_temp,
						aframe,
						start_projpts - edit_start,
						transition->length());
				}
			}
			if(playable_edit && start_projpts + fragment_duration >= edit_end)
				playable_edit = playable_edit->next;
		}
		start_projpts += fragment_duration;
	}

	return result;
}

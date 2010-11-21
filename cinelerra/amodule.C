
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
#include "aedit.h"
#include "amodule.h"
#include "aplugin.h"
#include "arender.h"
#include "asset.h"
#include "atrack.h"
#include "bcsignals.h"
#include "cache.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filexml.h"
#include "floatautos.h"
#include "language.h"
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
#include "transportque.h"
#include <string.h>



AModule::AModule(RenderEngine *renderengine, 
	CommonRender *commonrender, 
	PluginArray *plugin_array,
	Track *track)
 : Module(renderengine, commonrender, plugin_array, track)
{
	data_type = TRACK_AUDIO;
	transition_temp = 0;
	transition_temp_alloc = 0;
	level_history = 0;
	current_level = 0;
}

AModule::~AModule()
{
	if(transition_temp) delete [] transition_temp;
	if(level_history)
	{
		delete [] level_history;
		delete [] level_samples;
	}
}

AttachmentPoint* AModule::new_attachment(Plugin *plugin)
{
	return new AAttachmentPoint(renderengine, plugin);
}


void AModule::create_objects()
{
	Module::create_objects();
// Not needed in pluginarray
	if(commonrender)
	{
		level_history = new double[((ARender*)commonrender)->total_peaks];
		level_samples = new samplenum[((ARender*)commonrender)->total_peaks];
		current_level = 0;

		for(int i = 0; i < ((ARender*)commonrender)->total_peaks; i++)
		{
			level_history[i] = 0;
			level_samples[i] = -1;
		}
	}
}

int AModule::get_buffer_size()
{
	if(renderengine)
		return renderengine->fragment_len;
	else
		return plugin_array->get_bufsize();
}

void AModule::reverse_buffer(double *buffer, int len)
{
	int start, end;
	double temp;

	for(start = 0, end = len - 1; end > start; start++, end--)
	{
		temp = buffer[start];
		buffer[start] = buffer[end];
		buffer[end] = temp;
	}
}


CICache* AModule::get_cache()
{
	if(renderengine)
		return renderengine->get_acache();
	else
		return cache;
}

int AModule::render(double *buffer, 
	ptstime input_postime,
	int input_len, 
	int direction,
	int sample_rate,
	int use_nudge)
{
	int edl_rate = get_edl()->session->sample_rate;
	if(use_nudge) 
		input_postime += track->nudge;

	AEdit *playable_edit;
	ptstime length_pts = track->from_units(input_len);
	ptstime start_projpts = input_postime;
	ptstime end_projpts = start_projpts + length_pts;

	int buffer_offset = 0;
	int result = 0;

// Flip range around so start_project < end_project
	if(direction == PLAY_REVERSE)
	{
		start_projpts -= length_pts;
		end_projpts -= length_pts;
	}


// Clear buffer
	memset(buffer, 0, input_len * sizeof(double));

// Get first edit containing the range
	for(playable_edit = (AEdit*)track->edits->first; 
		playable_edit;
		playable_edit = (AEdit*)playable_edit->next)
	{
		ptstime edit_start = playable_edit->project_pts;
		ptstime edit_end = playable_edit->end_pts();

		if(start_projpts < edit_end && start_projpts + input_len > edit_start)
		{
			break;
		}
	}

// Fill output one fragment at a time
	while(start_projpts < end_projpts)
	{
		ptstime fragment_lenpts = length_pts;

		if(fragment_lenpts + start_projpts > end_projpts)
			fragment_lenpts = end_projpts - start_projpts;
		int fragment_len = track->to_units(fragment_lenpts, 1);

// Normalize position here since update_transition is a boolean operation.
		update_transition(start_projpts,
			PLAY_FORWARD);

		if(playable_edit)
		{
// Normalize EDL positions to requested rate
			ptstime edit_start = playable_edit->project_pts;
			ptstime edit_end = playable_edit->end_pts();
			ptstime edit_src = playable_edit->source_pts;

// Trim fragment_len
			if(fragment_lenpts + start_projpts > edit_end)
			{
				fragment_lenpts = edit_end - start_projpts;
				fragment_len = track->to_units(fragment_lenpts, 1);
			}
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
					int result = 0;

					result = source->set_audio_position(track->to_units(start_projpts - 
							edit_start +
							edit_src, 1),
						sample_rate);

					source->set_channel(playable_edit->channel);

					source->read_samples(buffer + buffer_offset, 
						fragment_len,
						sample_rate);

					get_cache()->check_in(playable_edit->asset);
				}
			}

// Read transition into temp and render
			AEdit *previous_edit = (AEdit*)playable_edit->previous;
			if(transition && previous_edit)
			{
				ptstime transition_len = transition->length();
				ptstime previous_start = previous_edit->project_pts;
				ptstime previous_src = previous_edit->source_pts;

// Read into temp buffers
// Temp + master or temp + temp ? temp + master
				if(transition_temp && transition_temp_alloc < fragment_len)
				{
					delete [] transition_temp;
					transition_temp = 0;
				}

				if(!transition_temp)
				{
					transition_temp = new double[fragment_len];
					transition_temp_alloc = fragment_len;
				}

// Trim transition_len
				int transition_fragment_len = fragment_len;
				if(fragment_lenpts + start_projpts > 
					edit_start + transition_len)
				{
					fragment_lenpts = edit_start + transition_len - start_projpts;
					fragment_len = track->to_units(fragment_lenpts, 1);
				}

				if(transition_fragment_len > 0)
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
							int result = 0;

							result = source->set_audio_position(track->to_units(start_projpts -
									previous_start +
									previous_src, 1),
								get_edl()->session->sample_rate);

							source->set_channel(previous_edit->channel);

							source->read_samples(transition_temp, 
								transition_fragment_len,
								sample_rate);

							get_cache()->check_in(previous_edit->asset);
						}
					}
					else
					{
						memset(transition_temp, 0, transition_fragment_len * sizeof(double));
					}

					double *output = buffer + buffer_offset;
					transition_server->process_transition(
						transition_temp,
						output,
						start_projpts - edit_start,
						transition_fragment_len,
						transition->length());
				}
			}

			if(playable_edit && start_projpts + fragment_lenpts >= edit_end)
				playable_edit = (AEdit*)playable_edit->next;
		}
		buffer_offset += fragment_len;
		start_projpts += fragment_lenpts;
	}

// Reverse buffer here so plugins always render forward.
	if(direction == PLAY_REVERSE)
		reverse_buffer(buffer, input_len);

	return result;
}

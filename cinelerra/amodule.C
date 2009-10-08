
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
#include "patch.h"
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
		level_samples = new int64_t[((ARender*)commonrender)->total_peaks];
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

void AModule::reverse_buffer(double *buffer, int64_t len)
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
	int64_t input_position,
	int input_len, 
	int direction,
	int sample_rate,
	int use_nudge)
{
	int64_t edl_rate = get_edl()->session->sample_rate;
	if(use_nudge) 
		input_position += track->nudge * 
			sample_rate /
			edl_rate;

	AEdit *playable_edit;
	int64_t start_project = input_position;
	int64_t end_project = input_position + input_len;
	int64_t buffer_offset = 0;
	int result = 0;


// Flip range around so start_project < end_project
	if(direction == PLAY_REVERSE)
	{
		start_project -= input_len;
		end_project -= input_len;
	}


// Clear buffer
	bzero(buffer, input_len * sizeof(double));

// The EDL is normalized to the requested sample rate because the requested rate may
// be the project sample rate and a sample rate 
// might as well be directly from the source rate to the requested rate.
// Get first edit containing the range
	for(playable_edit = (AEdit*)track->edits->first; 
		playable_edit;
		playable_edit = (AEdit*)playable_edit->next)
	{
		int64_t edit_start = playable_edit->startproject;
		int64_t edit_end = playable_edit->startproject + playable_edit->length;
// Normalize to requested rate
		edit_start = edit_start * sample_rate / edl_rate;
		edit_end = edit_end * sample_rate / edl_rate;

		if(start_project < edit_end && start_project + input_len > edit_start)
		{
			break;
		}
	}







// Fill output one fragment at a time
	while(start_project < end_project)
	{
		int64_t fragment_len = input_len;


		if(fragment_len + start_project > end_project)
			fragment_len = end_project - start_project;

// Normalize position here since update_transition is a boolean operation.
		update_transition(start_project * 
				edl_rate / 
				sample_rate, 
			PLAY_FORWARD);

		if(playable_edit)
		{
// Normalize EDL positions to requested rate
			int64_t edit_startproject = playable_edit->startproject;
			int64_t edit_endproject = playable_edit->startproject + playable_edit->length;
			int64_t edit_startsource = playable_edit->startsource;
//printf("AModule::render 52 %lld\n", fragment_len);

			edit_startproject = edit_startproject * sample_rate / edl_rate;
			edit_endproject = edit_endproject * sample_rate / edl_rate;
			edit_startsource = edit_startsource * sample_rate / edl_rate;
//printf("AModule::render 53 %lld\n", fragment_len);



// Trim fragment_len
			if(fragment_len + start_project > edit_endproject)
				fragment_len = edit_endproject - start_project;
//printf("AModule::render 56 %lld\n", fragment_len);

			if(playable_edit->asset)
			{
				File *source;
				get_cache()->age();



				if(!(source = get_cache()->check_out(playable_edit->asset,
					get_edl())))
				{
// couldn't open source file / skip the edit
					result = 1;
					printf(_("VirtualAConsole::load_track Couldn't open %s.\n"), playable_edit->asset->path);
				}
				else
				{
					int result = 0;


					result = source->set_audio_position(start_project - 
							edit_startproject + 
							edit_startsource, 
						sample_rate);

// 					if(result) printf("AModule::render start_project=%d playable_edit->startproject=%d playable_edit->startsource=%d\n"
// 						"source=%p playable_edit=%p edl=%p edlsession=%p sample_rate=%d\n",
// 						start_project, playable_edit->startproject, playable_edit->startsource, 
// 						source, playable_edit, get_edl(), get_edl()->session, get_edl()->session->sample_rate);

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
				int64_t transition_len = transition->length * 
					sample_rate / 
					edl_rate;
				int64_t previous_startproject = previous_edit->startproject *
					sample_rate /
					edl_rate;
				int64_t previous_startsource = previous_edit->startsource *
					sample_rate /
					edl_rate;

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
				if(fragment_len + start_project > 
					edit_startproject + transition_len)
					fragment_len = edit_startproject + transition_len - start_project;
//printf("AModule::render 54 %lld\n", fragment_len);

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
							printf(_("VirtualAConsole::load_track Couldn't open %s.\n"), playable_edit->asset->path);
							result = 1;
						}
						else
						{
							int result = 0;

							result = source->set_audio_position(start_project - 
									previous_startproject + 
									previous_startsource, 
								get_edl()->session->sample_rate);

// 							if(result) printf("AModule::render start_project=%d playable_edit->startproject=%d playable_edit->startsource=%d\n"
// 								"source=%p playable_edit=%p edl=%p edlsession=%p sample_rate=%d\n",
// 								start_project, 
// 								previous_edit->startproject, 
// 								previous_edit->startsource, 
// 								source, 
// 								playable_edit, 
// 								get_edl(), 
// 								get_edl()->session, 
// 								get_edl()->session->sample_rate);

							source->set_channel(previous_edit->channel);

							source->read_samples(transition_temp, 
								transition_fragment_len,
								sample_rate);

							get_cache()->check_in(previous_edit->asset);
						}
					}
					else
					{
						bzero(transition_temp, transition_fragment_len * sizeof(double));
					}

					double *output = buffer + buffer_offset;
					transition_server->process_transition(
						transition_temp,
						output,
						start_project - edit_startproject,
						transition_fragment_len,
						transition->length);
				}
			}

			if(playable_edit && start_project + fragment_len >= edit_endproject)
				playable_edit = (AEdit*)playable_edit->next;
		}

		buffer_offset += fragment_len;
		start_project += fragment_len;
	}


// Reverse buffer here so plugins always render forward.
	if(direction == PLAY_REVERSE)
		reverse_buffer(buffer, input_len);


	return result;
}










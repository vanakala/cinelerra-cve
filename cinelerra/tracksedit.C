
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

#include "atrack.h"
#include "automation.h"
#include "bcsignals.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "localsession.h"
#include "plugin.h"
#include "pluginserver.h"
#include "tracks.h"
#include "vtrack.h"

#include <string.h>

void Tracks::clear(ptstime start, ptstime end)
{
	Track *current_track;

	for(current_track = first; 
		current_track; 
		current_track = current_track->next)
	{
		if(current_track->record)
			current_track->clear(start, end);
	}
}

void Tracks::clear_automation(ptstime selectionstart, ptstime selectionend)
{
	Track* current_track;

	for(current_track = first; current_track; current_track = current_track->next)
	{
		if(current_track->record)
		{
			current_track->clear_automation(selectionstart, 
				selectionend);
		}
	}
}

void Tracks::straighten_automation(ptstime selectionstart, ptstime selectionend)
{
	Track* current_track;

	for(current_track = first; current_track; current_track = current_track->next)
	{
		if(current_track->record)
		{
			current_track->straighten_automation(selectionstart, 
				selectionend); 
		}
	}
}

void Tracks::clear_handle(ptstime start,
	ptstime end,
	ptstime &longest_distance,
	int actions)
{
	Track* current_track;
	ptstime distance;

	for(current_track = first; current_track; current_track = current_track->next)
	{
		if(current_track->record)
		{
			current_track->clear_handle(start, 
				end, 
				actions,
				distance);
			if(distance > longest_distance) longest_distance = distance;
		}
	}
}

void Tracks::automation_xml(FileXML *file)
{
// called by MWindow::copy_automation for copying automation alone
	Track* current_track;

	file->tag.set_title("AUTO_CLIPBOARD");
	file->append_tag();
	file->append_newline();

	for(current_track = first; 
		current_track; 
		current_track = current_track->next)
	{
		if(current_track->record)
		{
			current_track->automation_xml(file);
		}
	}

	file->tag.set_title("/AUTO_CLIPBOARD");
	file->append_tag();
	file->append_newline();
	file->terminate_string();
}

int Tracks::delete_tracks(void)
{
	int total_deleted = 0;
	int done = 0;

	while(!done)
	{
		done = 1;
		for (Track* current = first;
			current && done;
			current = NEXT)
		{
			if(current->record)
			{
				delete_track(current);
				total_deleted++;
				done = 0;
				break;
			}
		}
	}
	edl->check_master_track();
	return total_deleted;
}

void Tracks::move_effect(Plugin *plugin,
	Track *dest_track,
	ptstime dest_postime)
{
	Track *source_track = plugin->track;
	Plugin *result = 0;
// Insert on an existing plugin set
	if(!dest_track)
	{
		int i = plugin->get_number();
		int j;

		if(i > 0)
			j = i - 1;
		else
			j = i + 1;
		source_track->xchg_plugins(source_track->plugins.values[i],
			source_track->plugins.values[j]);
	}
	else
// Create a new plugin set
	{
		result = dest_track->insert_effect(0,
				dest_postime,
				plugin->get_length(),
				plugin->plugin_type,
				plugin->shared_plugin,
				plugin->shared_track);

		result->copy_from(plugin);
		result->shift(dest_postime - plugin->get_pts());
// Delete old plugin
		source_track->remove_plugin(plugin);
	}
}

void Tracks::concatenate_tracks()
{
	Track *output_track, *first_output_track, *input_track;
	int i, data_type = TRACK_AUDIO;
	ptstime output_start;
	Track *master_track = master();
	ptstime output_max = -1;

// Calculate new length
	if(master_track)
	{
		output_max = master_track->get_length();

		if(master_track->record)
		{
			for(output_track = first; output_track;
				output_track = output_track->next)
			{
				if(output_track->data_type == master_track->data_type &&
						!output_track->record &&
						output_track->play)
					output_max += output_track->get_length();
			}
		}
	}
// Relocate tracks
	for(i = 0; i < TRACK_TYPES; i++)
	{
// Get first output track
		for(output_track = first; output_track;
			output_track = output_track->next)
		{
			if(output_track->data_type == data_type && 
					output_track->record)
				break;
		}

		first_output_track = output_track;

// Get first input track
		for(input_track = first; input_track;
			input_track = input_track->next)
		{
			if(input_track->data_type == data_type &&
					input_track->play &&
					!input_track->record)
				break;
		}

		if(output_track && input_track)
		{
// Transfer input track to end of output track one at a time
			while(input_track)
			{
				output_start = output_track->get_length();

				if(output_start < output_max - EPSILON)
				{
					output_track->insert_track(input_track,
						input_track->get_length(),
						output_start);
					if(output_track->get_length() > output_max)
						output_track->clear_after(output_max);
				}

// Get next source and destination
				for(input_track = input_track->next;
					input_track;
					input_track = input_track->next)
				{

					if(input_track->data_type == data_type && 
							!input_track->record &&
							input_track->play)
						break;
				}

				for(output_track = output_track->next; 
					output_track; 
					output_track = output_track->next)
				{
					if(output_track->data_type == data_type && 
							output_track->record)
						break;
				}

				if(!output_track)
				{
					output_track = first_output_track;
				}
			}
		}

		if(data_type == TRACK_AUDIO)
			data_type = TRACK_VIDEO;
	}
}

void Tracks::delete_all_tracks()
{
	while(last) delete last;
}

// =========================================== EDL editing

void Tracks::save_xml(FileXML *file, const char *output_path)
{
	Track* current;

	for(current = first; current; current = NEXT)
		current->save_xml(file, output_path);
}

void Tracks::copy(Tracks *tracks, ptstime start, ptstime end,
	ArrayList<Track*> *src_tracks)
{
	Track *current, *new_track;

	for(current = tracks->first; current; current = NEXT)
	{
		if(src_tracks && src_tracks->number_of(current) < 0)
			continue;

		switch(current->data_type)
		{
		case TRACK_VIDEO:
			new_track = add_video_track(0, 0);
			break;

		case TRACK_AUDIO:
			new_track = add_audio_track(0, 0);
			break;

		default:
			new_track = 0;
			break;
		}

		if(new_track)
			new_track->copy(current, start, end);
	}
	init_plugin_pointers_by_ids();
}

void Tracks::move_track_up(Track *track)
{
	Track *next_track = track->previous;
	if(!next_track) next_track = last;

	swap(track, next_track);
}

void Tracks::move_track_down(Track *track)
{
	Track *next_track = track->next;
	if(!next_track) next_track = first;

	swap(track, next_track);
}

int Tracks::move_tracks_up()
{
	Track *track, *next_track;
	int result = 0;

	for(track = first;
		track; 
		track = next_track)
	{
		next_track = track->next;

		if(track->record)
		{
			if(track->previous)
			{
				swap(track->previous, track);
				result = 1;
			}
		}
	}

	return result;
}

int Tracks::move_tracks_down()
{
	Track *track, *previous_track;
	int result = 0;

	for(track = last;
		track; 
		track = previous_track)
	{
		previous_track = track->previous;

		if(track->record)
		{
			if(track->next)
			{
				swap(track, track->next);
				result = 1;
			}
		}
	}

	return result;
}

void Tracks::paste_audio_transition(PluginServer *server)
{
	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == TRACK_AUDIO &&
			current->record)
		{
			ptstime position = 
				edl->local_session->get_selectionstart();
			Edit *current_edit = current->edits->editof(position, 0);

			if(current_edit)
				paste_transition(server, current_edit);
		}
	}
}

void Tracks::paste_automation(ptstime selectionstart, FileXML *file)
{
	Track* current_atrack = 0;
	Track* current_vtrack = 0;
	int result = 0;
	char string[BCTEXTLEN];
	string[0] = 0;

// Search for start
	do{
		result = file->read_tag();
	}while(!result && !file->tag.title_is("AUTO_CLIPBOARD"));

	if(!result)
	{
		do
		{
			result = file->read_tag();

			if(!result)
			{
				if(file->tag.title_is("/AUTO_CLIPBOARD"))
				{
					result = 1;
				}
				else
				if(file->tag.title_is("TRACK"))
				{
					file->tag.get_property("TYPE", string);

					if(!strcmp(string, "AUDIO"))
					{
// Get next audio track
						if(!current_atrack)
							current_atrack = first;
						else
							current_atrack = current_atrack->next;

						while(current_atrack && 
							(current_atrack->data_type != TRACK_AUDIO ||
							!current_atrack->record))
							current_atrack = current_atrack->next;

// Paste it
						if(current_atrack)
						{
							current_atrack->paste_automation(
								selectionstart,
								file);
						}
					}
					else
					{
// Get next video track
						if(!current_vtrack)
							current_vtrack = first;
						else
							current_vtrack = current_vtrack->next;

						while(current_vtrack && 
							(current_vtrack->data_type != TRACK_VIDEO ||
							!current_vtrack->record))
							current_vtrack = current_vtrack->next;

// Paste it
						if(current_vtrack)
						{
							current_vtrack->paste_automation(
								selectionstart,
								file);
						}
					}
				}
			}
		}while(!result);
	}
}

void Tracks::paste_transition(PluginServer *server, Edit *dest_edit)
{
	dest_edit->insert_transition(server);
}

void Tracks::paste_video_transition(PluginServer *server, int first_track)
{
	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == TRACK_VIDEO &&
			current->record)
		{
			ptstime position = 
				edl->local_session->get_selectionstart();
			Edit *current_edit = current->edits->editof(position, 
				0);
			if(current_edit)
			{
				paste_transition(server, current_edit);
			}
			if(first_track) break;
		}
	}
}

void Tracks::paste_silence(ptstime start, ptstime end)
{
	Track* current_track;
	ptstime paste_end = 0;
	ptstime track_end = length();
	ptstime silence_len = end - start;

	if(silence_len < EPSILON)
		return;

	for(current_track = first; current_track;
		current_track = current_track->next)
	{
		if(!current_track->record)
			continue;
		if(current_track->master && current_track->record)
		{
			paste_end = end;
			if(start > track_end)
				start = track_end;
			break;
		}
		ptstime len = current_track->get_length();
		if(len + silence_len > track_end)
		{
			silence_len = track_end - len;
			paste_end = start + silence_len;
		}
	}

	if(paste_end < EPSILON || paste_end < start)
		return;

	for(current_track = first; current_track;
		current_track = current_track->next)
	{
		if(current_track->record)
			current_track->paste_silence(start, paste_end);
	}
}

ptstime Tracks::adjust_position(ptstime oldposition,
	ptstime newposition,
	int currentend,
	int handle_mode)
{
	for(Track *current = first; current; current = current->next)
	{
		newposition = current->adjust_position(oldposition,
			newposition, currentend, handle_mode);
	}
	return newposition;
}

void Tracks::modify_edithandles(ptstime oldposition,
	ptstime newposition,
	int currentend, 
	int handle_mode)
{
	Track *current;

	for(current = first; current; current = NEXT)
	{
		if(current->record)
		{
			current->modify_edithandles(oldposition,
				newposition, 
				currentend,
				handle_mode);
		}
	}
}

void Tracks::modify_pluginhandles(ptstime oldposition,
	ptstime newposition,
	int currentend, 
	int handle_mode)
{
	Track *current;

	for(current = first; current; current = NEXT)
	{
		if(current->record)
		{
			current->modify_pluginhandles(oldposition, 
				newposition, 
				currentend, 
				handle_mode);
		}
	}
}

Track *Tracks::master()
{
	for(Track *current = first; current; current = NEXT)
	{
		if(current->master)
			return current;
	}
	return 0;
}

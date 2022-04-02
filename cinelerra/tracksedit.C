// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "automation.h"
#include "bcsignals.h"
#include "edl.h"
#include "filexml.h"
#include "localsession.h"
#include "plugin.h"
#include "track.h"
#include "tracks.h"

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
	cleanup();
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

// Insert on an existing plugin set
	if(source_track != dest_track)
	{
		source_track->plugins.remove(plugin);
		dest_track->plugins.append(plugin);
		plugin->track = dest_track;
// Remove shared plugin if it is on same track
		for(int i = 0; i < dest_track->plugins.total; i++)
		{
			if(dest_track->plugins.values[i]->shared_plugin == plugin)
			{
				dest_track->remove_plugin(dest_track->plugins.values[i]);
				break;
			}
		}
	}

	ptstime max_start = dest_track->plugin_max_start(plugin);

	if(dest_postime > max_start)
		dest_postime = max_start;

	plugin->shift(dest_postime - plugin->get_pts());

	for(Track *track = first; track; track = track->next)
	{
		if(track == dest_track)
			continue;
		for(int i = 0; i < track->plugins.total; i++)
		{
			Plugin *current = track->plugins.values[i];

			if(current->shared_plugin == plugin)
				current->shift(dest_postime - current->get_pts());
		}
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
		output_max = master_track->duration();

		if(master_track->record)
		{
			for(output_track = first; output_track;
				output_track = output_track->next)
			{
				if(output_track->data_type == master_track->data_type &&
						!output_track->record &&
						output_track->play)
					output_max += output_track->duration();
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
				output_start = output_track->duration();

				if(output_start < output_max - EPSILON)
				{
					output_track->insert_track(input_track,
						input_track->duration(),
						output_start);
					if(output_track->duration() > output_max)
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
	cleanup();
}

void Tracks::delete_all_tracks()
{
	while(last) delete last;
}

ptstime Tracks::paste_duration(ptstime position, Asset *asset,
	Track *first_track, ptstime prev_duration)
{
	ptstime paste_end = position + prev_duration;

	for(Track *dest = first_track; dest; dest = dest->next)
	{
		if(dest->record)
		{
			int stream, channel, chtype;
			int astream = -1;
			int vstream = -1;
			int achannel = -1;
			int vchannel = -1;

			switch(dest->data_type)
			{
			case TRACK_AUDIO:
				stream = astream;
				channel = achannel;
				chtype = STRDSC_AUDIO;
				break;
			case TRACK_VIDEO:
				stream = vstream;
				channel = vchannel;
				chtype = STRDSC_VIDEO;
				break;
			}
			if(asset)
			{
				if(stream < 0 || channel < 0)
				{
					if((stream = asset->get_stream_ix(
							chtype, stream)) >= 0)
						channel = 0;
				}

				if(channel >= 0)
				{
					if(dest->master)
						paste_end = position +
							asset->duration();
					channel++;
					if(channel > asset->streams[stream].channels)
						channel = -1;
				}
			}
			switch(dest->data_type)
			{
			case TRACK_AUDIO:
				astream = stream;
				achannel = channel;
				break;
			case TRACK_VIDEO:
				vstream = stream;
				vchannel = channel;
				break;
			}
		}
	}
	return paste_end - position;
}

ptstime Tracks::paste_duration(ptstime position, EDL *clip,
	Track *first_track, ptstime prev_duration)
{
	Track *clipatrack, *clipvtrack, *cliptrack;
	ptstime paste_end = position + prev_duration;

	clipvtrack = clipatrack = clip->tracks->first;

	for(Track *dest = first_track; dest; dest = dest->next)
	{
		if(dest->record)
		{
			switch(dest->data_type)
			{
			case TRACK_AUDIO:
				cliptrack = clipatrack;
				break;
			case TRACK_VIDEO:
				cliptrack = clipvtrack;
				break;
			}
			while(cliptrack && cliptrack->data_type != dest->data_type)
				cliptrack = cliptrack->next;

			if(cliptrack)
			{
				if(dest->master)
					paste_end = position + cliptrack->duration();
				cliptrack = cliptrack->next;
			}

			switch(dest->data_type)
			{
			case TRACK_AUDIO:
				clipatrack = cliptrack;
				break;
			case TRACK_VIDEO:
				clipvtrack = cliptrack;
				break;
			}
		}
	}
	return paste_end - position;
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

		new_track = add_track(current->data_type, 0, 0);

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
	reset_plugins();
}

void Tracks::move_track_down(Track *track)
{
	Track *next_track = track->next;
	if(!next_track) next_track = first;

	swap(track, next_track);
	reset_plugins();
}

void Tracks::move_tracks_up()
{
	Track *track, *next_track;

	for(track = first;
		track; 
		track = next_track)
	{
		next_track = track->next;

		if(track->record)
		{
			if(track->previous)
				swap(track->previous, track);
		}
	}
	reset_plugins();
}

void Tracks::move_tracks_down()
{
	Track *track, *previous_track;

	for(track = last;
		track; 
		track = previous_track)
	{
		previous_track = track->previous;

		if(track->record)
		{
			if(track->next)
				swap(track, track->next);
		}
	}
	reset_plugins();
}

void Tracks::load_effects(FileXML *file, int operation)
{
	Track *current;
	char string[BCTEXTLEN];
	string[0] = 0;

// Search for start
	while(!file->read_tag() && file->tag.title_is("AUTO_CLIPBOARD"));

	do
	{
		if(file->tag.title_is("/AUTO_CLIPBOARD"))
			break;

		if(file->tag.title_is("TRACK"))
		{
			file->tag.get_property("TYPE", string);

			if(!strcmp(string, "VIDEO"))
				current = add_track(TRACK_VIDEO, 0, 0);

			if(!strcmp(string, "AUDIO"))
				current = add_track(TRACK_AUDIO, 0, 0);

			current->load_effects(file, operation);
		}
	}while(!file->read_tag());

	init_shared_pointers();
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
		ptstime len = current_track->duration();
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

Track *Tracks::master()
{
	for(Track *current = first; current; current = NEXT)
	{
		if(current->master)
			return current;
	}
	return 0;
}

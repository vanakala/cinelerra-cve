
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
#include "intauto.h"
#include "intautos.h"
#include "localsession.h"
#include "mainundo.h"
#include "module.h"
#include "mainsession.h"
#include "pluginserver.h"
#include "pluginset.h"
#include "timebar.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transition.h"
#include "vtrack.h"

#include <string.h>

void Tracks::clear(ptstime start, ptstime end, int clear_plugins)
{
	Track *current_track;

	for(current_track = first; 
		current_track; 
		current_track = current_track->next)
	{
		if(current_track->record) 
		{
			current_track->clear(start, 
				end, 
				EDIT_EDITS | EDIT_LABELS | (clear_plugins ? EDIT_PLUGINS : 0));
		}
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
				selectionend, 
				0,
				0); 
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
	file->tag.set_property("LENGTH", total_length());
	file->tag.set_property("FRAMERATE", edlsession->frame_rate);
	file->tag.set_property("SAMPLERATE", edlsession->sample_rate);
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
	return total_deleted;
}

void Tracks::move_effect(Plugin *plugin,
	PluginSet *dest_plugin_set,
	Track *dest_track, 
	ptstime dest_postime)
{
	Track *source_track = plugin->track;
	Plugin *result = 0;
// Insert on an existing plugin set
	if(!dest_track && dest_plugin_set)
	{
		if(plugin->plugin_set == dest_plugin_set)
		{
			ptstime opos = plugin->get_pts();
			ptstime length = plugin->length();
			dest_postime = plugin->set_pts(dest_postime);
			plugin->next->set_pts(dest_postime + length);
			plugin->shift_keyframes(dest_postime - opos);
		}
		else
			source_track->xchg_pluginsets(dest_plugin_set, plugin->plugin_set);
	}
	else
// Create a new plugin set
	{
		result = dest_track->insert_effect("", 
				&plugin->shared_location, 
				0,
				0,
				dest_postime,
				plugin->length(),
				plugin->plugin_type);

		result->copy_from(plugin);
		result->shift(dest_postime - plugin->get_pts());

// Clear new plugin from old set
		plugin->plugin_set->clear(plugin->get_pts(),
			plugin->end_pts());

		source_track->optimize();
	}
}

int Tracks::concatenate_tracks(int edit_plugins)
{
	Track *output_track, *first_output_track, *input_track;
	int i, data_type = TRACK_AUDIO;
	ptstime output_start;
	FileXML *clipboard;
	int result = 0;
	IntAuto *play_keyframe = 0;

// Relocate tracks
	for(i = 0; i < 2; i++)
	{
// Get first output track
		for(output_track = first; 
			output_track; 
			output_track = output_track->next)
			if(output_track->data_type == data_type && 
				output_track->record) break;

		first_output_track = output_track;

// Get first input track
		for(input_track = first;
			input_track;
			input_track = input_track->next)
		{
			if(input_track->data_type == data_type &&
				input_track->play && 
				!input_track->record) break;
		}

		if(output_track && input_track)
		{
// Transfer input track to end of output track one at a time
			while(input_track)
			{
				output_start = output_track->get_length();
				output_track->insert_track(input_track, 
					output_start, 
					0,
					edit_plugins);

// Get next source and destination
				for(input_track = input_track->next; 
					input_track; 
					input_track = input_track->next)
				{

					if(input_track->data_type == data_type && 
						!input_track->record && 
						input_track->play) break;
				}

				for(output_track = output_track->next; 
					output_track; 
					output_track = output_track->next)
				{
					if(output_track->data_type == data_type && 
						output_track->record) break;
				}

				if(!output_track)
				{
					output_track = first_output_track;
				}
			}
			result = 1;
		}

		if(data_type == TRACK_AUDIO) data_type = TRACK_VIDEO;
	}
	return result;
}

void Tracks::delete_all_tracks()
{
	while(last) delete last;
}

void Tracks::change_modules(int old_location, int new_location, int do_swap)
{
	for(Track* current = first ; current; current = current->next)
		current->change_modules(old_location, new_location, do_swap);
}

void Tracks::change_plugins(SharedLocation &old_location, SharedLocation &new_location, int do_swap)
{
	for(Track* current = first ; current; current = current->next)
		current->change_plugins(old_location, new_location, do_swap);
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
}

void Tracks::move_track_up(Track *track)
{
	Track *next_track = track->previous;
	if(!next_track) next_track = last;

	change_modules(number_of(track), number_of(next_track), 1);

	swap(track, next_track);
}

void Tracks::move_track_down(Track *track)
{
	Track *next_track = track->next;
	if(!next_track) next_track = first;

	change_modules(number_of(track), number_of(next_track), 1);
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
				change_modules(number_of(track->previous), number_of(track), 1);

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
				change_modules(number_of(track), number_of(track->next), 1);

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

void Tracks::paste_automation(ptstime selectionstart, 
	FileXML *file,
	int default_only)
{
	Track* current_atrack = 0;
	Track* current_vtrack = 0;
	int result = 0;
	ptstime length;
	double frame_rate = edlsession->frame_rate;
	int sample_rate = edlsession->sample_rate;
	char string[BCTEXTLEN];
	string[0] = 0;

// Search for start
	do{
		result = file->read_tag();
	}while(!result && 
		!file->tag.title_is("AUTO_CLIPBOARD"));

	if(!result)
	{
		length = file->tag.get_property("LENGTH_TIME", (ptstime)0);
		frame_rate = file->tag.get_property("FRAMERATE", frame_rate);
		sample_rate = file->tag.get_property("SAMPLERATE", sample_rate);

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
							current_atrack->paste_automation(selectionstart,
								length,
								frame_rate,
								sample_rate,
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
							current_vtrack->paste_automation(selectionstart,
								length,
								frame_rate,
								sample_rate,
								file);
						}
					}
				}
			}
		}while(!result);
	}
}

void Tracks::paste_default_keyframe(FileXML *file)
{
	paste_automation(0, file, 1);
}

void Tracks::paste_transition(PluginServer *server, Edit *dest_edit)
{
	dest_edit->insert_transition(server->title, &server->default_keyframe);
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

void Tracks::paste_silence(ptstime start, ptstime end, int edit_plugins)
{
	Track* current_track;

	for(current_track = first; 
		current_track; 
		current_track = current_track->next)
	{
		if(current_track->record) 
			current_track->paste_silence(start, end, edit_plugins); 
	}
}

void Tracks::modify_edithandles(ptstime &oldposition,
	ptstime &newposition,
	int currentend, 
	int handle_mode,
	int actions)
{
	Track *current;

	for(current = first; current; current = NEXT)
	{
		if(current->record)
		{
			current->modify_edithandles(oldposition, 
				newposition, 
				currentend,
				handle_mode,
				actions);
		}
	}
}

void Tracks::modify_pluginhandles(ptstime &oldposition,
	ptstime &newposition,
	int currentend, 
	int handle_mode,
	int edit_labels)
{
	Track *current;

	for(current = first; current; current = NEXT)
	{
		if(current->record)
		{
			current->modify_pluginhandles(oldposition, 
				newposition, 
				currentend, 
				handle_mode,
				edit_labels);
		}
	}
}

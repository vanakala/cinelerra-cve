#include "assets.h"
#include "atrack.h"
#include "automation.h"
#include "aedits.h"
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
#include "trackscroll.h"
#include "transition.h"
#include "transportque.h"
#include "vtrack.h"
#include <string.h>

int Tracks::clear(double start, double end, int clear_plugins)
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
				1, 
				1, 
				clear_plugins, 
				1); 
		}
	}
	return 0;
}

void Tracks::clear_automation(double selectionstart, double selectionend)
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

int Tracks::clear_default_keyframe()
{
	for(Track *current = first; current; current = NEXT)
	{
		if(current->record)
			current->clear_automation(0, 0, 0, 1);
	}
	return 0;
}

int Tracks::clear_handle(double start, 
	double end,
	double &longest_distance,
	int clear_labels,
	int clear_plugins)
{
	Track* current_track;
	double distance;

	for(current_track = first; current_track; current_track = current_track->next)
	{
		if(current_track->record)
		{
			current_track->clear_handle(start, 
				end, 
				clear_labels,
				clear_plugins, 
				distance);
			if(distance > longest_distance) longest_distance = distance;
		}
	}

	return 0;
}

int Tracks::copy_automation(double selectionstart, 
	double selectionend, 
	FileXML *file,
	int default_only,
	int autos_only)
{
// called by MWindow::copy_automation for copying automation alone
	Track* current_track;

	file->tag.set_title("AUTO_CLIPBOARD");
	file->tag.set_property("LENGTH", selectionend - selectionstart);
	file->tag.set_property("FRAMERATE", edl->session->frame_rate);
	file->tag.set_property("SAMPLERATE", edl->session->sample_rate);
	file->append_tag();
	file->append_newline();
	file->append_newline();

	for(current_track = first; 
		current_track; 
		current_track = current_track->next)
	{
		if(current_track->record)
		{
			current_track->copy_automation(selectionstart, 
				selectionend, 
				file,
				default_only,
				autos_only);
		}
	}

	file->tag.set_title("/AUTO_CLIPBOARD");
	file->append_tag();
	file->append_newline();
	file->terminate_string();
	return 0;
}

int Tracks::copy_default_keyframe(FileXML *file)
{
	copy_automation(0, 0, file, 1, 0);
	return 0;
}

int Tracks::delete_tracks()
{
	int result = 1, total_deleted = 0;
	Track *track, *next_track, *shifted_track;
	int deleted_number;
	
	while(result)
	{
// keep deleting until all the recordable tracks are gone
		result = 0;

//printf("Tracks::delete_tracks 1\n");
// Stop when first recordable track is reached
		for(track = first; 
			track && !result;  
			track = next_track)
		{
//printf("Tracks::delete_tracks 2\n");
			next_track = track->next;
			
			if(track->record)
			{
				deleted_number = number_of(track);
// Delete the track.
				delete track;

// Shift all the plugin pointers.
				for(shifted_track = next_track;
					shifted_track;
					shifted_track = shifted_track->next)
				{
					shifted_track->delete_module_pointers(deleted_number);
				}
				result = 1;
				total_deleted++;
			}
		}
//printf("Tracks::delete_tracks 3\n");
	}
//printf("Tracks::delete_tracks 4\n");
	return total_deleted;
}

void Tracks::move_edits(ArrayList<Edit*> *edits, 
	Track *track,
	double position,
	int edit_labels,  // Ignored
	int edit_plugins)  // Ignored
{
	int current_aedit = 0;
	int current_vedit = 0;

//printf("Tracks::move_edits 1\n");
	for(Track *dest_track = track; dest_track; dest_track = dest_track->next)
	{
		if(dest_track->record)
		{
// Need a local copy of the source edit since the original source edit may
// change in the editing operation.
			Edit *source_edit = 0;
			Track *source_track = 0;
// Get source track
			if(dest_track->data_type == TRACK_AUDIO &&
				current_aedit < edits->total)
			{
				while(current_aedit < edits->total &&
					edits->values[current_aedit]->track->data_type != TRACK_AUDIO)
					current_aedit++;

				if(current_aedit < edits->total)
				{
					source_edit = edits->values[current_aedit];
					source_track = source_edit->track;
					current_aedit++;
				}
			}
			else
			if(dest_track->data_type == TRACK_VIDEO &&
				current_vedit < edits->total)
			{
				while(current_vedit < edits->total &&
					edits->values[current_vedit]->track->data_type != TRACK_VIDEO)
					current_vedit++;

				if(current_vedit < edits->total)
				{
					source_edit = edits->values[current_vedit];
					source_track = source_edit->track;
					current_vedit++;
				}
			}

//printf("Tracks::move_edits 2 %s %s %d\n", source_track->title, dest_track->title, source_edit->length);
			if(source_edit)
			{
// Copy keyframes
				FileXML temp;
				AutoConf temp_autoconf;
				int64_t position_i = source_track->to_units(position, 0);
// Source edit changes
				int64_t source_length = source_edit->length;

				temp_autoconf.set_all();
				source_track->automation->copy(source_edit->startproject, 
					source_edit->startproject + source_edit->length, 
					&temp, 
					0,
					0);
				temp.terminate_string();
				temp.rewind();


//printf("Tracks::move_edits 1 %d\n", source_edit->length);
// Insert new edit
				Edit *dest_edit = dest_track->edits->shift(position_i, 
					source_length);
				Edit *result = dest_track->edits->insert_before(dest_edit, 
					new Edit(edl, dest_track));
				result->copy_from(source_edit);
				result->startproject = position_i;
				result->length = source_length;

// Insert new keyframes
//printf("Tracks::move_edits 2 %d %p\n", result->startproject, result->asset);
				dest_track->automation->paste_silence(source_edit->startproject, 
					source_edit->startproject + source_length);
				while(!temp.read_tag())
					dest_track->automation->paste(position_i, 
						source_length, 
						1.0, 
						&temp, 
						0,
						&temp_autoconf);

//printf("Tracks::move_edits 5\n");
//dest_track->dump();

// Clear source
				int64_t clear_start = source_edit->startproject;
				int64_t clear_end = clear_start + source_length;

//printf("Tracks::move_edits 7 %d %d\n", clear_start, clear_end);
				source_track->edits->clear(clear_start, 
					clear_end);
				source_track->automation->clear(clear_start,
					clear_end,
					&temp_autoconf,
					1);


//printf("Tracks::move_edits 8 %d %d\n", clear_start, source_edit->length);
//dest_track->dump();
//printf("Tracks::move_edits 9\n");
				source_track->optimize();
				dest_track->optimize();
//printf("Tracks::move_edits 10\n");
//dump();
//				delete source_edit;
			}

		}
	}
}

void Tracks::move_effect(Plugin *plugin,
	PluginSet *dest_plugin_set,
	Track *dest_track, 
	int64_t dest_position)
{
	Track *source_track = plugin->track;
	Plugin *result = 0;

// Insert on an existing plugin set
	if(!dest_track && dest_plugin_set)
	{
		Track *dest_track = dest_plugin_set->track;


// Assume this operation never splits a plugin
// Shift destination plugins back
		dest_plugin_set->shift(dest_position, plugin->length);

// Insert new plugin
		Plugin *current = 0;
		for(current = (Plugin*)dest_plugin_set->first; current; current = (Plugin*)NEXT)
			if(current->startproject >= dest_position) break;

		result = (Plugin*)dest_plugin_set->insert_before(current, 
			new Plugin(edl, dest_plugin_set, ""));
	}
	else
// Create a new plugin set
	{
		double length = 0;
		double start = 0;
		if(edl->local_session->get_selectionend() > 
			edl->local_session->get_selectionstart())
		{
			start = edl->local_session->get_selectionstart();
			length = edl->local_session->get_selectionend() - 
				start;
		}
		else
		if(dest_track->get_length() > 0)
		{
			start = 0;
			length = dest_track->get_length();
		}
		else
		{
			start = 0;
			length = dest_track->from_units(plugin->length);
		}


		result = dest_track->insert_effect("", 
				&plugin->shared_location, 
				0,
				0,
				start,
				length,
				plugin->plugin_type);
	}



	result->copy_from(plugin);
	result->shift(dest_position - plugin->startproject);

// Clear new plugin from old set
	plugin->plugin_set->clear(plugin->startproject, plugin->startproject + plugin->length);


	source_track->optimize();
}



int Tracks::concatenate_tracks(int edit_plugins)
{
	Track *output_track, *first_output_track, *input_track;
	int i, data_type = TRACK_AUDIO;
	double output_start;
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

int Tracks::delete_audio_track()
{
	Track *current;

	for(current = last; current && current->data_type != TRACK_AUDIO; current = PREVIOUS)
	{
		;
	}

	if(current) delete_track(current);
}

int Tracks::delete_video_track()
{
	Track *current;

	for(current = last; current && current->data_type != TRACK_VIDEO; current = PREVIOUS)
	{
		;
	}

	if(current) delete_track(current);
}

int Tracks::delete_all_tracks()
{
	while(last) delete last;
	return 0;
}


void Tracks::change_modules(int old_location, int new_location, int do_swap)
{
	for(Track* current = first ; current; current = current->next)
	{
		current->change_modules(old_location, new_location, do_swap);
	}
}

void Tracks::change_plugins(SharedLocation &old_location, SharedLocation &new_location, int do_swap)
{
	for(Track* current = first ; current; current = current->next)
	{
		current->change_plugins(old_location, new_location, do_swap);
	}
}



// =========================================== EDL editing


int Tracks::copy(double start, 
	double end, 
	int all, 
	FileXML *file, 
	char *output_path)
{
// nothing selected
	if(start == end && !all) return 1;

	Track* current;

	for(current = first; 
		current; 
		current = NEXT)
	{
		if(current->record || all)
		{
			current->copy(start, end, file,output_path);
		}
	}

	return 0;
}



int Tracks::move_track_up(Track *track)
{
	Track *next_track = track->previous;
	if(!next_track) next_track = last;

	change_modules(number_of(track), number_of(next_track), 1);

// printf("Tracks::move_track_up 1 %p %p\n", track, next_track);
// int count = 0;
// for(Track *current = first; current && count < 5; current = NEXT, count++)
// 	printf("Tracks::move_track_up %p %p %p\n", current->previous, current, current->next);
// printf("Tracks::move_track_up 2\n");
// 
	swap(track, next_track);

// count = 0;
// for(Track *current = first; current && count < 5; current = NEXT, count++)
// 	printf("Tracks::move_track_up %p %p %p\n", current->previous, current, current->next);
// printf("Tracks::move_track_up 3\n");

	return 0;
}

int Tracks::move_track_down(Track *track)
{
	Track *next_track = track->next;
	if(!next_track) next_track = first;

	change_modules(number_of(track), number_of(next_track), 1);
	swap(track, next_track);
	return 0;
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
			int64_t position = current->to_units(
				edl->local_session->get_selectionstart(), 0);
			Edit *current_edit = current->edits->editof(position, PLAY_FORWARD);
			if(current_edit)
			{
				paste_transition(server, current_edit);
			}
		}
	}
}

void Tracks::paste_automation(double selectionstart, 
	FileXML *file,
	int default_only)
{
	Track* current_atrack = 0;
	Track* current_vtrack = 0;
	int result = 0;
	double length;
	double frame_rate = edl->session->frame_rate;
	int64_t sample_rate = edl->session->sample_rate;
	char string[BCTEXTLEN];
	sprintf(string, "");

// Search for start
	do{
	  result = file->read_tag();
	}while(!result && 
		!file->tag.title_is("AUTO_CLIPBOARD"));

	if(!result)
	{
		length = file->tag.get_property("LENGTH", 0);
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
								file,
								default_only);
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
//printf("Tracks::paste_automation 1 %s %d\n", current_vtrack->title, current_vtrack->record);
							current_vtrack->paste_automation(selectionstart,
								length,
								frame_rate,
								sample_rate,
								file,
								default_only);
						}
					}
				}
			}
		}while(!result);
	}
}

int Tracks::paste_default_keyframe(FileXML *file)
{
	paste_automation(0, file, 1);
	return 0;
}

void Tracks::paste_transition(PluginServer *server, Edit *dest_edit)
{
	dest_edit->insert_transition(server->title);
}

void Tracks::paste_video_transition(PluginServer *server, int first_track)
{
	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == TRACK_VIDEO &&
			current->record)
		{
			int64_t position = current->to_units(
				edl->local_session->get_selectionstart(), 0);
			Edit *current_edit = current->edits->editof(position, PLAY_FORWARD);
			if(current_edit)
			{
				paste_transition(server, current_edit);
			}
			if(first_track) break;
		}
	}
}


int Tracks::paste_silence(double start, double end, int edit_plugins)
{
	Track* current_track;

	for(current_track = first; 
		current_track; 
		current_track = current_track->next)
	{
		if(current_track->record) 
		{ 
			current_track->paste_silence(start, end, edit_plugins); 
		}
	}
	return 0;
}



int Tracks::select_auto(int cursor_x, int cursor_y)
{
	int result = 0;
	for(Track* current = first; current && !result; current = NEXT) { result = current->select_auto(&auto_conf, cursor_x, cursor_y); }
	return result;
}

int Tracks::move_auto(int cursor_x, int cursor_y, int shift_down)
{
	int result = 0;

	for(Track* current = first; current && !result; current = NEXT) 
	{
		result = current->move_auto(&auto_conf, cursor_x, cursor_y, shift_down); 
	}
	return 0;
}

int Tracks::modify_edithandles(double &oldposition, 
	double &newposition, 
	int currentend, 
	int handle_mode,
	int edit_labels,
	int edit_plugins)
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
				edit_labels,
				edit_plugins);
		}
	}
	return 0;
}

int Tracks::modify_pluginhandles(double &oldposition, 
	double &newposition, 
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
	return 0;
}



int Tracks::purge_asset(Asset *asset)
{
	Track *current_track;
	int result = 0;
	
	for(current_track = first; current_track; current_track = current_track->next)
	{
		result += current_track->purge_asset(asset); 
	}
	return result;
}

int Tracks::asset_used(Asset *asset)
{
	Track *current_track;
	int result = 0;
	
	for(current_track = first; current_track; current_track = current_track->next)
	{
		result += current_track->asset_used(asset); 
	}
	return result;
}

int Tracks::scale_time(float rate_scale, int ignore_record, int scale_edits, int scale_autos, int64_t start, int64_t end)
{
	Track *current_track;

	for(current_track = first; 
		current_track; 
		current_track = current_track->next)
	{
		if((current_track->record || ignore_record) && 
			current_track->data_type == TRACK_VIDEO)
		{
			current_track->scale_time(rate_scale, scale_edits, scale_autos, start, end);
		}
	}
	return 0;
}


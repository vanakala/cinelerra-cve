#include "aedit.h"
#include "asset.h"
#include "assets.h"
#include "automation.h"
#include "cache.h"
#include "clip.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filexml.h"
#include "filesystem.h"
#include "localsession.h"
#include "plugin.h"
#include "strategies.inc"
#include "track.h"
#include "transition.h"
#include "transportque.inc"

#include <string.h>

Edits::Edits(EDL *edl, Track *track)
 : List<Edit>()
{
	this->edl = edl;
	this->track = track;
}

Edits::~Edits()
{
}


void Edits::equivalent_output(Edits *edits, int64_t *result)
{
// For the case of plugin sets, a new plugin set may be created with
// plugins only starting after 0.  We only want to restart brender at
// the first plugin in this case.
	for(Edit *current = first, *that_current = edits->first; 
		current || that_current; 
		current = NEXT,
		that_current = that_current->next)
	{
//printf("Edits::equivalent_output 1 %d\n", *result);
		if(!current && that_current)
		{
			int64_t position1 = (last ? last->startproject + last->length : 0);
			int64_t position2 = that_current->startproject;
			if(*result < 0 || *result > MIN(position1, position2))
				*result = MIN(position1, position2);
			break;
		}
		else
		if(current && !that_current)
		{
			int64_t position1 = (edits->last ? edits->last->startproject + edits->last->length : 0);
			int64_t position2 = current->startproject;
			if(*result < 0 || *result > MIN(position1, position2))
				*result = MIN(position1, position2);
			break;
		}
		else
		{
//printf("Edits::equivalent_output 2 %d\n", *result);
			current->equivalent_output(that_current, result);
//printf("Edits::equivalent_output 3 %d\n", *result);
		}
	}
}


Edits& Edits::operator=(Edits& edits)
{
	while(last) delete last;
	for(Edit *current = edits.first; current; current = NEXT)
	{
		Edit *new_edit = append(create_edit());
		new_edit->copy_from(current);
	}
	return *this;
}


void Edits::insert_asset(Asset *asset,
	int64_t length,
	int64_t position,
	int track_number)
{
	Edit *new_edit = insert_new_edit(position);

	new_edit->asset = asset;
	new_edit->startsource = 0;
	new_edit->startproject = position;
	new_edit->length = length;

	if(asset->audio_data)
		new_edit->channel = track_number % asset->channels;
	else
	if(asset->video_data)
		new_edit->channel = track_number % asset->layers;

//printf("Edits::insert_asset %d %d\n", new_edit->channel, new_edit->length);
	for(Edit *current = new_edit->next; current; current = NEXT)
	{
		current->startproject += length;
	}
}

void Edits::insert_edits(Edits *source_edits, int64_t position)
{
	int64_t clipboard_length = 
		track->to_units(source_edits->edl->local_session->clipboard_length, 1);
	int64_t clipboard_end = position + clipboard_length;


// Fill region between end of edit table and beginning of pasted segment
// with silence.  Can't call from insert_new_edit because it's recursive.
	if(position > length())
	{
		paste_silence(length(), position);
	}


	for(Edit *source_edit = source_edits->first;
		source_edit;
		source_edit = source_edit->next)
	{
// Update Assets
		Asset *dest_asset = edl->assets->update(source_edit->asset);
// Open destination area
		Edit *dest_edit = insert_new_edit(position + source_edit->startproject);

		dest_edit->copy_from(source_edit);
		dest_edit->asset = dest_asset;
		dest_edit->startproject = position + source_edit->startproject;



// Shift keyframes in source edit to their position in the
// destination edit for plugin case
		dest_edit->shift_keyframes(position);



// Shift following edits and keyframes in following edits by length
// in current source edit.
		for(Edit *future_edit = dest_edit->next;
			future_edit;
			future_edit = future_edit->next)
		{
			future_edit->startproject += dest_edit->length;
			future_edit->shift_keyframes(dest_edit->length);
		}

// Fill clipboard length with silence
		if(!source_edit->next && 
			dest_edit->startproject + dest_edit->length < clipboard_end)
		{
			paste_silence(dest_edit->startproject + dest_edit->length,
				clipboard_end);
		}
	}
}


// Native units
// Can't paste silence in here because it's used by paste_silence.
Edit* Edits::insert_new_edit(int64_t position)
{
	Edit *current = 0;
//printf("Edits::insert_new_edit 1\n");
	current = split_edit(position);
	if(current) current = PREVIOUS;

//printf("Edits::insert_new_edit 1\n");
	Edit *new_edit = create_edit();
//printf("Edits::insert_new_edit 1\n");
	insert_after(current, new_edit);
	new_edit->startproject = position;
//printf("Edits::insert_new_edit 2\n");
	return new_edit;
}


Edit* Edits::split_edit(int64_t position)
{
// Get edit containing position
	Edit *edit = editof(position, PLAY_FORWARD, 0);

// No edit found
	if(!edit)
	{
		return 0;
	}
// Split would have created a 0 length
//	if(edit->startproject == position) return edit;
// Create anyway so the return value comes before position

	Edit *new_edit = create_edit();
	insert_after(edit, new_edit);
	new_edit->copy_from(edit);
	new_edit->length = new_edit->startproject + new_edit->length - position;
	edit->length = position - edit->startproject;
	new_edit->startproject = edit->startproject + edit->length;
	new_edit->startsource += edit->length;


// Decide what to do with the transition
	if(edit->length && edit->transition)
	{
		delete new_edit->transition;
		new_edit->transition = 0;
	}

	if(edit->transition && edit->transition->length > edit->length) 
		edit->transition->length = edit->length;
	if(new_edit->transition && new_edit->transition->length > new_edit->length)
		new_edit->transition->length = new_edit->length;
	return new_edit;
}

int Edits::save(FileXML *xml, char *output_path)
{
	copy(0, length(), xml, output_path);
	return 0;
}

void Edits::resample(double old_rate, double new_rate)
{
	for(Edit *current = first; current; current = NEXT)
	{
		current->startproject = Units::to_int64((double)current->startproject / 
			old_rate * 
			new_rate);
		if(PREVIOUS) PREVIOUS->length = current->startproject - PREVIOUS->startproject;
		current->startsource = Units::to_int64((double)current->startsource /
			old_rate *
			new_rate);
		if(!NEXT) current->length = Units::to_int64((double)current->length /
			old_rate *
			new_rate);
		current->resample(old_rate, new_rate);
	}
}














int Edits::optimize()
{
	int result = 1;
	Edit *current;

//return 0;
// Sort edits by starting point
	while(result)
	{
		result = 0;
		
		for(current = first; current; current = NEXT)
		{
			Edit *next_edit = NEXT;
			
			if(next_edit && next_edit->startproject < current->startproject)
			{
				swap(next_edit, current);
				result = 1;
			}
		}
	}

// Insert silence between edits which aren't consecutive
	for(current = last; current; current = current->previous)
	{
		if(current->previous)
		{
			Edit *previous_edit = current->previous;
			if(current->startproject - 
				previous_edit->startproject -
				previous_edit->length > 0)
			{
				Edit *new_edit = create_edit();
				insert_before(current, new_edit);
				new_edit->startproject = previous_edit->startproject + previous_edit->length;
				new_edit->length = current->startproject - 
					previous_edit->startproject -
					previous_edit->length;
			}
		}
		else
		if(current->startproject > 0)
		{
			Edit *new_edit = create_edit();
			insert_before(current, new_edit);
			new_edit->length = current->startproject;
		}
	}

	result = 1;
	while(result)
	{
		result = 0;


// delete 0 length edits
		for(current = first; 
			current && !result; )
		{
			if(current->length == 0)
			{
				Edit* next = current->next;
				delete current;
				result = 1;
				current = next;
			}
			else
				current = current->next;
		}

// merge same files or transitions
		for(current = first; 
			current && current->next && !result; )
		{
// assets identical
			Edit *next_edit = current->next;
    		if(current->asset == next_edit->asset && 
    		   	(!current->asset ||
					(current->startsource + current->length == next_edit->startsource &&
	       			current->channel == next_edit->channel)
				)
			)
        	{       
// source positions are consecutive
        		current->length += next_edit->length;
        		remove(next_edit);
        		result = 1;
        	}

    		current = (Plugin*)current->next;
		}

// delete last edit of 0 length or silence
		if(last && 
			(last->silence() || 
			!last->length))
		{
			delete last;
			result = 1;
		}
	}

//track->dump();
	return 0;
}




















// ===================================== file operations

int Edits::load(FileXML *file, int track_offset)
{
	int result = 0;
	int64_t startproject = 0;

	do{
		result = file->read_tag();

//printf("Edits::load 1 %s\n", file->tag.get_title());
		if(!result)
		{
			if(!strcmp(file->tag.get_title(), "EDIT"))
			{
				load_edit(file, startproject, track_offset);
			}
			else
			if(!strcmp(file->tag.get_title(), "/EDITS"))
			{
				result = 1;
			}
		}
	}while(!result);

//track->dump();
	optimize();
}

int Edits::load_edit(FileXML *file, int64_t &startproject, int track_offset)
{
	Edit* current;

//printf("Edits::load_edit 1 %d\n", total());
	current = append_new_edit();
//printf("Edits::load_edit 2 %d\n", total());

	current->load_properties(file, startproject);

	startproject += current->length;

	int result = 0;
//printf("Edits::load_edit 1\n");

	do{
//printf("Edits::load_edit 2\n");
		result = file->read_tag();
//printf("Edits::load_edit 3 %s\n", file->tag.get_title());

		if(!result)
		{
			if(file->tag.title_is("FILE"))
			{
				char filename[1024];
				sprintf(filename, SILENCE);
				file->tag.get_property("SRC", filename);
// Extend path
//printf("Edits::load_edit 4 %s\n", filename);
				if(strcmp(filename, SILENCE))
				{
					char directory[BCTEXTLEN], edl_directory[BCTEXTLEN];
					FileSystem fs;
					fs.set_current_dir("");
					fs.extract_dir(directory, filename);
					if(!strlen(directory))
					{
						fs.extract_dir(edl_directory, file->filename);
						fs.join_names(directory, edl_directory, filename);
						strcpy(filename, directory);
					}
					current->asset = edl->assets->get_asset(filename);
				}
				else
				{
					current->asset = edl->assets->get_asset(SILENCE);
				}
//printf("Edits::load_edit 5\n");
			}
			else
			if(file->tag.title_is("TRANSITION"))
			{
				current->transition = new Transition(edl,
					current, 
					"",
					track->to_units(edl->session->default_transition_length, 1));
				current->transition->load_xml(file);
			}
			else
			if(file->tag.title_is(SILENCE))
			{
//printf("Edits::load_edit 6\n");
				current->asset = edl->assets->get_asset(SILENCE);
//printf("Edits::load_edit 7\n");
			}
			else
			if(file->tag.title_is("/EDIT"))
			{
				result = 1;
			}
		}
//printf("Edits::load_edit 8\n");
	}while(!result);

//printf("Edits::load_edit 5\n");
// in case of incomplete edit tag
	if(!current->asset) current->asset = edl->assets->get_asset(SILENCE);
	return 0;
}

// ============================================= accounting

int64_t Edits::length()
{
	if(last) 
		return last->startproject + last->length;
	else 
		return 0;
}
// 
// int64_t Edits::total_length() 
// {
// 	int64_t total = 0;
// 	Edit* current;
// 	for(current = first; current; current = NEXT)
// 	{
// 		total += current->length;
// 	}
// 	return total; 
// };

Edit* Edits::editof(int64_t position, int direction, int use_nudge)
{
	Edit *current = 0;
	if(use_nudge && track) position += track->nudge;

	if(direction == PLAY_FORWARD)
	{
		for(current = last; current; current = PREVIOUS)
		{
			if(current->startproject <= position && current->startproject + current->length > position)
				return current;
		}
	}
	else
	if(direction == PLAY_REVERSE)
	{
		for(current = first; current; current = NEXT)
		{
			if(current->startproject < position && current->startproject + current->length >= position)
				return current;
		}
	}

	return 0;     // return 0 on failure
}

Edit* Edits::get_playable_edit(int64_t position, int use_nudge)
{
	Edit *current;
	if(track && use_nudge) position += track->nudge;

// Get the current edit
	for(current = first; current; current = NEXT)
	{
		if(current->startproject <= position && 
			current->startproject + current->length > position)
			break;
	}

// Get the edit's asset
	if(current)
	{
		if(!current->asset)
			current = 0;
	}

	return current;     // return 0 on failure
}

// ================================================ editing



int Edits::copy(int64_t start, int64_t end, FileXML *file, char *output_path)
{
	Edit *current_edit;

	file->tag.set_title("EDITS");
	file->append_tag();
	file->append_newline();

	for(current_edit = first; current_edit; current_edit = current_edit->next)
	{
		current_edit->copy(start, end, file, output_path);
	}

	file->tag.set_title("/EDITS");
	file->append_tag();
	file->append_newline();
}



void Edits::clear(int64_t start, int64_t end)
{
	Edit* edit1 = editof(start, PLAY_FORWARD, 0);
	Edit* edit2 = editof(end, PLAY_FORWARD, 0);
	Edit* current_edit;

	if(end == start) return;        // nothing selected
	if(!edit1 && !edit2) return;       // nothing selected


	if(!edit2)
	{                // edit2 beyond end of track
		edit2 = last;
		end = this->length();
	}

	if(edit1 != edit2)
	{
// in different edits

//printf("Edits::clear 3.5 %d %d %d %d\n", edit1->startproject, edit1->length, edit2->startproject, edit2->length);
		edit1->length = start - edit1->startproject;
		edit2->length -= end - edit2->startproject;
		edit2->startsource += end - edit2->startproject;
		edit2->startproject += end - edit2->startproject;

// delete
		for(current_edit = edit1->next; current_edit && current_edit != edit2;)
		{
			Edit* next = current_edit->next;
			remove(current_edit);
			current_edit = next;
		}
// shift
		for(current_edit = edit2; current_edit; current_edit = current_edit->next)
		{
			current_edit->startproject -= end - start;
		}
	}
	else
	{
// in same edit. paste_edit depends on this
// create a new edit
		current_edit = split_edit(start);

		current_edit->length -= end - start;
		current_edit->startsource += end - start;

// shift
		for(current_edit = current_edit->next; 
			current_edit; 
			current_edit = current_edit->next)
		{            
			current_edit->startproject -= end - start;
		}
	}

	optimize();
}

// Used by edit handle and plugin handle movement but plugin handle movement
// can only effect other plugins.
void Edits::clear_recursive(int64_t start, 
	int64_t end, 
	int edit_edits,
	int edit_labels, 
	int edit_plugins)
{
//printf("Edits::clear_recursive 1\n");
	track->clear(start, 
		end, 
		edit_edits,
		edit_labels,
		edit_plugins,
		0);
}


int Edits::clear_handle(double start, 
	double end, 
	int edit_plugins, 
	double &distance)
{
	Edit *current_edit;


	for(current_edit = first; 
		current_edit && current_edit->next; 
		current_edit = current_edit->next)
	{



		if(current_edit->asset && 
			current_edit->next->asset)
		{

			if(current_edit->asset->equivalent(*current_edit->next->asset,
				0,
				0))
			{

// Got two consecutive edits in same source
				if(edl->equivalent(track->from_units(current_edit->next->startproject), 
					start))
				{
// handle selected
					int length = -current_edit->length;
					current_edit->length = current_edit->next->startsource - current_edit->startsource;
					length += current_edit->length;

// Lengthen automation
					track->automation->paste_silence(current_edit->next->startproject, 
						current_edit->next->startproject + length);

// Lengthen effects
					if(edit_plugins)
						track->shift_effects(current_edit->next->startproject, 
							length,
							0);

					for(current_edit = current_edit->next; current_edit; current_edit = current_edit->next)
					{
						current_edit->startproject += length;
					}

					distance = track->from_units(length);
					optimize();
					break;
				}
			}
		}
	}

	return 0;
}

int Edits::modify_handles(double oldposition, 
	double newposition, 
	int currentend,
	int edit_mode, 
	int edit_edits,
	int edit_labels,
	int edit_plugins)
{
	int result = 0;
	Edit *current_edit;

//printf("Edits::modify_handles 1 %d %f %f\n", currentend, newposition, oldposition);
	if(currentend == 0)
	{
// left handle
		for(current_edit = first; current_edit && !result;)
		{
//printf("Edits::modify_handles 2 %f %f\n", 
//track->from_units(current_edit->startproject), 
//oldposition);
			if(edl->equivalent(track->from_units(current_edit->startproject), 
				oldposition))
			{
// edit matches selection
//printf("Edits::modify_handles 3 %f %f\n", newposition, oldposition);
				oldposition = track->from_units(current_edit->startproject);
				result = 1;

				if(newposition >= oldposition)
				{
//printf("Edits::modify_handle 1 %s %f %f\n", track->title, oldposition, newposition);
// shift start of edit in
					current_edit->shift_start_in(edit_mode, 
						track->to_units(newposition, 0), 
						track->to_units(oldposition, 0),
						edit_edits,
						edit_labels,
						edit_plugins);
				}
				else
				{
//printf("Edits::modify_handle 2 %s\n", track->title);
// move start of edit out
					current_edit->shift_start_out(edit_mode, 
						track->to_units(newposition, 0), 
						track->to_units(oldposition, 0),
						edit_edits,
						edit_labels,
						edit_plugins);
				}
			}

			if(!result) current_edit = current_edit->next;
		}
	}
	else
	{
// right handle selected
		for(current_edit = first; current_edit && !result;)
		{
//printf("Edits::modify_handle 1 %s\n", track->title);
			if(edl->equivalent(track->from_units(current_edit->startproject) + 
				track->from_units(current_edit->length), oldposition))
			{
//printf("Edits::modify_handle 2\n");
            	oldposition = track->from_units(current_edit->startproject) + 
					track->from_units(current_edit->length);
				result = 1;

//printf("Edits::modify_handle 3\n");
				if(newposition <= oldposition)
				{     // shift end of edit in
//printf("Edits::modify_handle 4\n");
					current_edit->shift_end_in(edit_mode, 
						track->to_units(newposition, 0), 
						track->to_units(oldposition, 0),
						edit_edits,
						edit_labels,
						edit_plugins);
//printf("Edits::modify_handle 5\n");
				}
				else
				{     // move end of edit out
//printf("Edits::modify_handle 6\n");
					current_edit->shift_end_out(edit_mode, 
						track->to_units(newposition, 0), 
						track->to_units(oldposition, 0),
						edit_edits,
						edit_labels,
						edit_plugins);
//printf("Edits::modify_handle 7\n");
				}
			}

			if(!result) current_edit = current_edit->next;
//printf("Edits::modify_handle 8\n");
		}
	}

	optimize();
	return 0;
}


// Used by other editing commands so don't optimize
Edit* Edits::paste_silence(int64_t start, int64_t end)
{
	Edit *new_edit = insert_new_edit(start);
	new_edit->length = end - start;
	for(Edit *current = new_edit->next; current; current = NEXT)
	{
		current->startproject += end - start;
	}
	return new_edit;
}
				     
Edit* Edits::shift(int64_t position, int64_t difference)
{
	Edit *new_edit = split_edit(position);

	for(Edit *current = first; 
		current; 
		current = NEXT)
	{
		if(current->startproject >= position)
		{
			current->shift(difference);
		}
	}
	return new_edit;
}


void Edits::shift_keyframes_recursive(int64_t position, int64_t length)
{
	track->shift_keyframes(position, length, 0);
}

void Edits::shift_effects_recursive(int64_t position, int64_t length)
{
	track->shift_effects(position, length, 0);
}


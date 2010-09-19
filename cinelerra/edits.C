
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

#include "aedit.h"
#include "asset.h"
#include "assets.h"
#include "automation.h"
#include "bcsignals.h"
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
#include "mainerror.h"
#include "plugin.h"
#include "strategies.inc"
#include "track.h"
#include "transition.h"
#include "transportque.inc"

#include <string.h>

Edits::Edits(EDL *edl, Track *track, Edit *default_edit)
 : List<Edit>()
{
	this->edl = edl;
	this->track = track;

	default_edit->edl = edl;
	default_edit->track = track;
	default_edit->project_pts = 0;
	default_edit->length_time = LAST_VIRTUAL_LENGTH;
	insert_after(0, default_edit);
	loaded_length = 0;
}

Edits::~Edits()
{
}


void Edits::equivalent_output(Edits *edits, ptstime *result)
{
// For the case of plugin sets, a new plugin set may be created with
// plugins only starting after 0.  We only want to restart brender at
// the first plugin in this case.
	for(Edit *current = first, *that_current = edits->first; 
		current || that_current; 
		current = NEXT,
		that_current = that_current->next)
	{
		if(!current && that_current)
		{
			ptstime position1 = length();
			ptstime position2 = that_current->project_pts;
			if(*result < 0 || *result > MIN(position1, position2))
				*result = MIN(position1, position2);
			break;
		}
		else
		if(current && !that_current)
		{
			ptstime position1 = edits->length();
			ptstime position2 = current->project_pts;
			if(*result < 0 || *result > MIN(position1, position2))
				*result = MIN(position1, position2);
			break;
		}
		else
		{
			current->equivalent_output(that_current, result);
		}
	}
}

void Edits::copy_from(Edits *edits)
{
	while(last) delete last;
	for(Edit *current = edits->first; current; current = NEXT)
	{
		Edit *new_edit = append(create_edit());
		new_edit->copy_from(current);
	}
	loaded_length = edits->loaded_length;
}


Edits& Edits::operator=(Edits& edits)
{
	copy_from(&edits);
	return *this;
}


void Edits::insert_asset(Asset *asset,
	ptstime length_time,
	ptstime postime,
	int track_number)
{
	Edit *new_edit = insert_new_edit(postime);

	new_edit->asset = asset;
	new_edit->source_pts = 0;
	new_edit->project_pts = postime;
	new_edit->length_time = length_time;

	if(asset->audio_data)
		new_edit->channel = track_number % asset->channels;
	else
	if(asset->video_data)
		new_edit->channel = track_number % asset->layers;

	for(Edit *current = new_edit->next; current; current = NEXT)
	{
		current->project_pts += length_time;
	}
}

void Edits::insert_edits(Edits *source_edits, ptstime postime)
{
	ptstime clipboard_time =
		source_edits->edl->local_session->clipboard_length;
	ptstime clipboard_end = postime + clipboard_time;

	ptstime total_time = 0;
	for(Edit *source_edit = source_edits->first;
		source_edit;
		source_edit = source_edit->next)
	{
// Update Assets
		Asset *dest_asset = edl->assets->update(source_edit->asset);
// Open destination area
		Edit *dest_edit = insert_new_edit(postime + source_edit->project_pts);

		dest_edit->copy_from(source_edit);
		dest_edit->asset = dest_asset;
		dest_edit->project_pts = postime + source_edit->project_pts;

// Shift keyframes in source edit to their position in the
// destination edit for plugin case
		dest_edit->shift_keyframes(postime);


		total_time += dest_edit->length_time;
		if (source_edits->loaded_length && total_time > source_edits->loaded_length)
		{
			dest_edit->length_time -= (total_time - source_edits->loaded_length);
		}

// Shift following edits and keyframes in following edits by length
// in current source edit.
		for(Edit *future_edit = dest_edit->next;
			future_edit;
			future_edit = future_edit->next)
		{
			future_edit->project_pts += dest_edit->length_time;
			future_edit->shift_keyframes(dest_edit->length_time);
		}

// Fill clipboard length with silence
		if(!source_edit->next && 
			dest_edit->project_pts + dest_edit->length_time < clipboard_end)
		{
			paste_silence(dest_edit->project_pts + dest_edit->length_time,
				clipboard_end);
		}
	}
}


// Can't paste silence in here because it's used by paste_silence.
Edit* Edits::insert_new_edit(ptstime postime)
{
	Edit *current = 0;
	Edit *new_edit;
	current = split_edit(postime);

	if (current->length_time == 0) // when creating a split we got 0-length edit, just use it!
		new_edit = current;
	else      // we need to insert 
	{
		current = PREVIOUS;
		new_edit = create_edit();
		insert_after(current, new_edit);
	}
	new_edit->project_pts = postime;
	return new_edit;
}


Edit* Edits::split_edit(ptstime postime)
{
// Get edit containing position
	Edit *edit = editof(postime, 0);
// No edit found, make one - except when we are at zero position!
	if(!edit && postime != 0)
		if (length() == postime)
		{
			edit = last; // we do not need any edit to extend past the last one
		} else
		if (!last || length() < postime)
		{
// Even when track is completely empty or split is beyond last edit, return correct edit
			Edit *empty = create_edit();
			if (last)
				empty->project_pts = length(); // end of last edit
			else
				empty->project_pts = 0; // empty track
			empty->length_time = postime - empty->project_pts;
			insert_after(last, empty);
			edit = empty;
		} else
		{
// now we are now surely in situation where we have 
//    a) broken edit list or b) negative position... report error!
			errorbox("Trying to insert edit at position, but failed: %.3f\n", postime);
			return 0;
		}

// Split would have created a 0 length
// Create anyway so the return value comes before position

	Edit *new_edit = create_edit();
	insert_after(edit, new_edit);
	if (edit)  // if we have actually split the edit, do the funky stuff!
	{
		new_edit->copy_from(edit);
		new_edit->length_time = new_edit->project_pts + new_edit->length_time - postime;
		edit->length_time = postime - edit->project_pts;
		new_edit->source_pts += edit->length_time;

// Decide what to do with the transition
		if(edit->length_time && edit->transition)
		{
			delete new_edit->transition;
			new_edit->transition = 0;
		}

		if(edit->transition && edit->transition->length_time > edit->length_time)
			edit->transition->length_time = edit->length_time;
		if(new_edit->transition && new_edit->transition->length_time > new_edit->length_time)
			new_edit->transition->length_time = new_edit->length_time;
	} else
		new_edit->length_time = 0;
	new_edit->project_pts = postime;
	return new_edit;
}

int Edits::save(FileXML *xml, const char *output_path)
{
	copy(0, length(), xml, output_path);
	return 0;
}


int Edits::optimize()
{
	int result = 1;
	Edit *current;

// Sort edits by starting point
	while(result)
	{
		result = 0;

		for(current = first; current; current = NEXT)
		{
			Edit *next_edit = NEXT;
			
			if(next_edit && next_edit->project_pts < current->project_pts)
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
			if(current->project_pts -
				previous_edit->project_pts -
				previous_edit->length_time > 0)
			{
				Edit *new_edit = create_edit();
				insert_before(current, new_edit);
				new_edit->project_pts = previous_edit->project_pts + previous_edit->length_time;
				new_edit->length_time = current->project_pts -
					previous_edit->project_pts -
					previous_edit->length_time;
			}
		}
		else
		if(current->project_pts > 0)
		{
			Edit *new_edit = create_edit();
			insert_before(current, new_edit);
			new_edit->length_time = current->project_pts;
		}
	}

	result = 1;
	while(result)
	{
		result = 0;

// delete 0 length edits
		for(current = first; 
			current != last && !result; )
		{
			if(PTSEQU(current->length_time, 0))
			{
				Edit* next = current->next;
				// Be smart with transitions!
				if (next && current->transition && !next->transition)
				{
					next->transition = current->transition;
					next->transition->edit = next;
					current->transition = 0;
				}
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
				(PTSEQU(current->source_pts + current->length_time, next_edit->source_pts) &&
				current->channel == next_edit->channel)))
			{
// source positions are consecutive
				current->length_time += next_edit->length_time;
				remove(next_edit);
				result = 1;
			}

			current = (Plugin*)current->next;
		}

// delete last edit of 0 length or silence
	}
	if (!last || !last->silence())
	{
// No last empty edit available... create one
		Edit *empty_edit = create_edit();
		if (!last) 
			empty_edit->project_pts = 0;
		else
			empty_edit->project_pts = last->project_pts + last->length_time;
		empty_edit->length_time = LAST_VIRTUAL_LENGTH;
		insert_after(last, empty_edit);
	}
	else
	{
		last->length_time = LAST_VIRTUAL_LENGTH;
	}

	return 0;
}


// ===================================== file operations

int Edits::load(FileXML *file, int track_offset)
{
	int result = 0;
	ptstime project_time = 0;
	while (last) 
		delete last;
	do{
		result = file->read_tag();

		if(!result)
		{
			if(!strcmp(file->tag.get_title(), "EDIT"))
			{
				load_edit(file, project_time, track_offset);
			}
			else
			if(!strcmp(file->tag.get_title(), "/EDITS"))
			{
				result = 1;
			}
		}
	}while(!result);

	if (last)
		loaded_length = last->project_pts + last->length_time;
	else 
		loaded_length = 0;
	optimize();
}

int Edits::load_edit(FileXML *file, ptstime &project_time, int track_offset)
{
	Edit* current;

	current = append_new_edit();
	current->load_properties(file, project_time);
	project_time += current->length_time;
	int result = 0;

	do{
		result = file->read_tag();

		if(!result)
		{
			if(file->tag.title_is("FILE"))
			{
				char filename[1024];
				strcpy(filename, SILENCE);
				file->tag.get_property("SRC", filename);
// Extend path
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
			}
			else
			if(file->tag.title_is("TRANSITION"))
			{
				current->transition = new Transition(edl,
					current, 
					"",
					edl->session->default_transition_length);
				current->transition->load_xml(file);
			}
			else
			if(file->tag.title_is(SILENCE))
			{
				current->asset = edl->assets->get_asset(SILENCE);
			}
			else
			if(file->tag.title_is("/EDIT"))
			{
				result = 1;
			}
		}
	}while(!result);

// in case of incomplete edit tag
	if(!current->asset) current->asset = edl->assets->get_asset(SILENCE);
	return 0;
}

// ============================================= accounting

ptstime Edits::length()
{
	if(last) 
		return last->project_pts + last->length_time;
	else 
		return 0;
}

Edit* Edits::editof(ptstime postime, int use_nudge)
{
	Edit *current = 0;
	if(use_nudge && track) postime += track->nudge;

	for(current = first; current; current = NEXT)
	{
		if(current->project_pts <= postime && current->project_pts + current->length_time > postime)
			return current;
	}

	return 0;     // return 0 on failure
}

Edit* Edits::get_playable_edit(ptstime postime, int use_nudge)
{
	Edit *current;
	if(track && use_nudge) postime += track->nudge;

// Get the current edit
	for(current = first; current; current = NEXT)
	{
		if(current->project_pts <= postime && 
			current->project_pts + current->length_time > postime)
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

int Edits::copy(ptstime start, ptstime end, FileXML *file, const char *output_path)
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


void Edits::clear(ptstime start, ptstime end)
{
	Edit* edit1 = editof(start, 0);
	Edit* edit2 = editof(end, 0);
	Edit* current_edit;

	if(PTSEQU(end, start)) return;        // nothing selected
	if(!edit1 && !edit2) return;       // nothing selected

	if(!edit2)
	{                // edit2 beyond end of track
		edit2 = last;
		end = this->length();
	}

	if(edit1 != edit2)
	{
// in different edits

		edit1->length_time = start - edit1->project_pts;
		edit2->length_time -= end - edit2->project_pts;
		edit2->source_pts += end - edit2->project_pts;
		edit2->project_pts += end - edit2->project_pts;

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
			current_edit->project_pts -= end - start;
		}
	}
	else
	{
// in same edit. paste_edit depends on this
// create a new edit
		current_edit = split_edit(start);

		current_edit->length_time -= end - start;
		current_edit->source_pts += end - start;

// shift
		for(current_edit = current_edit->next; 
			current_edit; 
			current_edit = current_edit->next)
		{            
			current_edit->project_pts -= end - start;
		}
	}

	optimize();
}

// Used by edit handle and plugin handle movement but plugin handle movement
// can only effect other plugins.
void Edits::clear_recursive(ptstime start,
	ptstime end,
	int edit_edits,
	int edit_labels,
	int edit_plugins,
	Edits *trim_edits)
{
	track->clear(start, 
		end, 
		edit_edits,
		edit_labels,
		edit_plugins,
		trim_edits);
}


int Edits::clear_handle(double start, 
	double end, 
	int edit_plugins, 
	ptstime &distance)
{
	Edit *current_edit;

	distance = 0.0; // if nothing is found, distance is 0!
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
				if(edl->equivalent(current_edit->next->project_pts,
					start))
				{
// handle selected
					ptstime length = -current_edit->length_time;
					current_edit->length_time = current_edit->next->source_pts - current_edit->source_pts;
					length += current_edit->length_time;

// Lengthen automation
					track->automation->paste_silence(current_edit->next->project_pts,
						current_edit->next->project_pts + length);

// Lengthen effects
					if(edit_plugins)
						track->shift_effects(current_edit->next->project_pts,
							length);

					for(current_edit = current_edit->next; current_edit; current_edit = current_edit->next)
					{
						current_edit->project_pts += length;
					}

					distance = length;
					optimize();
					break;
				}
			}
		}
	}

	return 0;
}

int Edits::modify_handles(ptstime oldposition,
	ptstime newposition,
	int currentend,
	int edit_mode, 
	int edit_edits,
	int edit_labels,
	int edit_plugins,
	Edits *trim_edits)
{
	int result = 0;
	Edit *current_edit;

	if(currentend == 0)
	{
// left handle
		for(current_edit = first; current_edit && !result;)
		{
			if(edl->equivalent(current_edit->project_pts,
				oldposition))
			{
// edit matches selection
				oldposition = current_edit->project_pts;
				result = 1;

				if(newposition >= oldposition)
				{
// shift start of edit in
					current_edit->shift_start_in(edit_mode, 
						newposition,
						oldposition,
						edit_edits,
						edit_labels,
						edit_plugins,
						trim_edits);
				}
				else
				{
// move start of edit out
					current_edit->shift_start_out(edit_mode,
						newposition,
						oldposition,
						edit_edits,
						edit_labels,
						edit_plugins,
						trim_edits);
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
			if(edl->equivalent(current_edit->project_pts + 
				current_edit->length_time, oldposition))
			{
				oldposition = current_edit->project_pts +
					current_edit->length_time;
				result = 1;

				if(newposition <= oldposition)
				{
// shift end of edit in
					current_edit->shift_end_in(edit_mode, 
						newposition,
						oldposition,
						edit_edits,
						edit_labels,
						edit_plugins,
						trim_edits);
				}
				else
				{
// move end of edit out
					current_edit->shift_end_out(edit_mode, 
						newposition,
						oldposition,
						edit_edits,
						edit_labels,
						edit_plugins,
						trim_edits);
				}
			}

			if(!result) current_edit = current_edit->next;
		}
	}

	optimize();
	return 0;
}


// Paste silence should not return anything - since pasting silence to an empty track should produce no edits
// If we need rutine to create new edit by pushing others forward, write new rutine and call it properly
// This are two distinctive functions
// This rutine leaves edits in optimized state!
void Edits::paste_silence(ptstime start, ptstime end)
{
	// paste silence does not do anything if 
	// a) paste silence is on empty track
	// b) paste silence is after last edit
	// in both cases editof returns NULL
	Edit *new_edit = editof(start, 0);
	if (!new_edit) return;

	if (!new_edit->asset)
	{ // in this case we extend already existing edit
		new_edit->length_time += end - start;
	} else
	{ // we are in fact creating a new edit
		new_edit = insert_new_edit(start);
		new_edit->length_time = end - start;
	}
	for(Edit *current = new_edit->next; current; current = NEXT)
	{
		current->project_pts += end - start;
	}
	return;
}

// Used by other editing commands so don't optimize
// This is separate from paste_silence, since it has to wrok also on empty tracks/beyond end of track
Edit *Edits::create_and_insert_edit(ptstime start, ptstime end)
{
	Edit *new_edit = insert_new_edit(start);
	new_edit->length_time = end - start;
	for(Edit *current = new_edit->next; current; current = NEXT)
	{
		current->project_pts += end - start;
	}
	return new_edit;
}

Edit* Edits::shift(ptstime position, ptstime difference)
{
	Edit *new_edit = split_edit(position);

	for(Edit *current = first; 
		current; 
		current = NEXT)
	{
		if(current->project_pts >= position)
		{
			current->shift(difference);
		}
	}
	return new_edit;
}


void Edits::shift_keyframes_recursive(ptstime position, ptstime length)
{
	track->shift_keyframes(position, length);
}

void Edits::shift_effects_recursive(ptstime position, ptstime length)
{
	track->shift_effects(position, length);
}


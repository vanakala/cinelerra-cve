
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
#include "track.h"
#include "transition.h"

#include <string.h>

Edits::Edits(EDL *edl, Track *track)
 : List<Edit>()
{
	this->edl = edl;
	this->track = track;
	loaded_length = 0;
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
	ptstime len_time,
	ptstime postime,
	int track_number)
{
	Edit *new_edit = insert_edit(postime, len_time);
	new_edit->asset = asset;
	new_edit->source_pts = 0;

	if(asset->audio_data)
		new_edit->channel = track_number % asset->channels;
	else
	if(asset->video_data)
		new_edit->channel = track_number % asset->layers;
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
		Edit *dest_edit = insert_edit(postime + source_edit->project_pts);

		dest_edit->copy_from(source_edit);
		dest_edit->asset = dest_asset;
		dest_edit->project_pts = postime + source_edit->project_pts;

// Shift keyframes in source edit to their position in the
// destination edit for plugin case
		dest_edit->shift_keyframes(postime);

		ptstime new_length = source_edit->length();
		total_time += new_length;
		if (source_edits->loaded_length && total_time > source_edits->loaded_length)
		{
			new_length -= (total_time - source_edits->loaded_length);
		}

// Shift following edits and keyframes in following edits by length
// in current source edit.
		for(Edit *future_edit = dest_edit->next;
			future_edit;
			future_edit = future_edit->next)
		{
			future_edit->project_pts += new_length;
			future_edit->shift_keyframes(new_length);
		}

// Fill clipboard length with silence
		if(!source_edit->next && 
			dest_edit->end_pts() < clipboard_end)
		{
			paste_silence(dest_edit->end_pts(),
				clipboard_end);
		}
	}
}

// Inserts a new edit with requested length
Edit* Edits::insert_edit(ptstime pts, ptstime length)
{
	Edit *new_edit = split_edit(pts);

	if(length >= track->one_unit)
	{
		ptstime end = pts + length;

		if(new_edit->next)
		{
			if(new_edit->next->project_pts < end)
			{
				ptstime shift = end - new_edit->next->project_pts;
				for(Edit *e = new_edit->next; e; e = e->next)
					e->project_pts += shift;
			}
			else
				split_edit(pts + length);
		}
		else
		{
			Edit *le = create_edit();
			append(le);
			le->project_pts = end;
		}
	}
	return new_edit;
}

Edit* Edits::split_edit(ptstime postime)
{
	Edit *edit;
	if(!first)
	{
		// List is empty create edit at 0, next edit at postime
		append(edit = create_edit());
		if(postime > EPSILON)
		{
			append(edit = create_edit());
			edit->project_pts = postime;
		}
		return edit;
	}
// Get edit containing position
	edit = editof(postime, 0);

	if(!edit)
	{
		// List ends before postime - create a new
		edit = create_edit();
		append(edit);
		edit->project_pts = postime;
		return edit;
	}

	if(PTSEQU(edit->project_pts, postime))
		return edit;

// Split existing edit
	Edit *new_edit = create_edit();
	Transition *trans = edit->transition;
	edit->transition = 0;

	insert_after(edit, new_edit);
	new_edit->copy_from(edit);
	new_edit->project_pts = postime;
	new_edit->source_pts += edit->length();

	if(trans)
	{
		if(edit->length() < trans->length_time)
			trans->length_time = edit->length();
		edit->transition = trans;
	}
	return new_edit;
}

void Edits::save(FileXML *xml, const char *output_path)
{
	copy(0, length(), xml, output_path);
}

// ===================================== file operations

void Edits::load(FileXML *file, int track_offset)
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
		loaded_length = last->end_pts();
	else 
		loaded_length = 0;
}

void Edits::load_edit(FileXML *file, ptstime &project_time, int track_offset)
{
	Edit* current;

	current = append(create_edit());
	project_time += current->load_properties(file, project_time);
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
					edl->session->default_transition_length, 0);
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
	if(!current->asset)
		current->asset = edl->assets->get_asset(SILENCE);
}

// ============================================= accounting

ptstime Edits::length()
{
	if(last) 
		return last->end_pts();
	else 
		return 0;
}

Edit* Edits::editof(ptstime postime, int use_nudge)
{
	Edit *current = 0;
	if(use_nudge && track) postime += track->nudge;

	for(current = first; current; current = NEXT)
	{
		if(PTSEQU(current->project_pts, postime) ||
				(current->project_pts <= postime &&
				current->end_pts() > postime))
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
				current->end_pts() > postime)
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

void Edits::copy(ptstime start, ptstime end, FileXML *file, const char *output_path)
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
	if(!edit1) return;       // nothing selected

	if(!edit2)
	{                // edit2 beyond end of track
		edit2 = last;
		end = this->length();
	}

	ptstime len = end - start;
	if(edit1 != edit2)
	{
// in different edits
		edit2->source_pts += end - edit2->project_pts;
		edit2->project_pts += end - edit2->project_pts;
// delete
		for(current_edit = edit1->next; current_edit && current_edit != edit2;)
		{
			Edit* next = current_edit->next;
			remove(current_edit);
			current_edit = next;
		}
		current_edit = edit2;
	}
	else
	{
		if(start > edit1->source_pts)
			current_edit = split_edit(start);
		else
			current_edit = edit1;
		current_edit->source_pts += len;
		current_edit = current_edit->next;
	}

	for(; current_edit; current_edit = current_edit->next)
	{
		if(current_edit->project_pts < len)
			current_edit->project_pts = 0;
		else
			current_edit->project_pts -= len;
	}
}

// Used by edit handle and plugin handle movement but plugin handle movement
// can only effect other plugins.
void Edits::clear_recursive(ptstime start,
	ptstime end,
	int actions,
	Edits *trim_edits)
{
	track->clear(start, 
		end, 
		actions,
		trim_edits);
}

void Edits::clear_handle(ptstime start,
	ptstime end, 
	int actions,
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
					ptstime length = -current_edit->length();
					current_edit->next->project_pts = current_edit->next->source_pts - current_edit->source_pts + 
						current_edit->project_pts;
					length += current_edit->length();

// Lengthen automation
					track->automation->paste_silence(current_edit->next->project_pts,
						current_edit->next->project_pts + length);

// Lengthen effects
					if(actions & EDIT_PLUGINS)
						track->shift_effects(current_edit->next->project_pts,
							length);

					for(current_edit = current_edit->next; current_edit; current_edit = current_edit->next)
					{
						current_edit->project_pts += length;
					}

					distance = length;
					break;
				}
			}
		}
	}
}

void Edits::modify_handles(ptstime &oldposition,
	ptstime &newposition,
	int currentend,
	int edit_mode, 
	int actions,
	Edits *trim_edits)
{
	int result = 0;
	Edit *current_edit;

	if(currentend == HANDLE_LEFT)
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
						actions,
						trim_edits);
				}
				else
				{
// move start of edit out
					current_edit->shift_start_out(edit_mode,
						newposition,
						oldposition,
						actions,
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
			if(edl->equivalent(current_edit->end_pts(), oldposition))
			{
				oldposition = current_edit->end_pts();
				result = 1;

				if(newposition <= oldposition)
				{
// shift end of edit in
					current_edit->shift_end_in(edit_mode, 
						newposition,
						oldposition,
						actions,
						trim_edits);
				}
				else
				{
// move end of edit out
					current_edit->shift_end_out(edit_mode, 
						newposition,
						oldposition,
						actions,
						trim_edits);
				}
			}

			if(!result) current_edit = current_edit->next;
		}
	}
}

// Paste silence should not return anything - since pasting silence to an empty track should produce no edits
// If we need rutine to create new edit by pushing others forward, write new rutine and call it properly
// This are two distinctive functions
void Edits::paste_silence(ptstime start, ptstime end)
{
	// paste silence does not do anything if 
	// a) paste silence is on empty track
	// b) paste silence is after last edit
	// in both cases editof returns NULL

	Edit *new_edit = editof(start, 0);
	if(!new_edit) return;

	if(new_edit->asset)
	{ // we are in fact creating a new edit
		new_edit = insert_edit(start, end - start);
		new_edit->asset = 0;
		new_edit->source_pts = 0;
		if(new_edit->next)
			new_edit->next->source_pts -= end - start;
	}
	if(new_edit->next)
	{
		new_edit = new_edit->next;
		for(Edit *current = new_edit->next; current; current = NEXT)
			current->project_pts += end - start;
	}
}

Edit* Edits::shift(ptstime position, ptstime difference)
{
	Edit *new_edit = split_edit(position);

	for(Edit *current = new_edit->next;
		current; 
		current = NEXT)
	{
		current->shift(difference);
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

void Edits::dump(int indent)
{
	Edit *edit;

	printf("%*sEdits %p dump(%d):\n", indent, "", this, total());
	for(edit = first; edit; edit = edit->next)
		edit->dump(indent + 2);
}

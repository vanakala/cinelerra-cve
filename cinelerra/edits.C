
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
	for(Edit *source_edit = source_edits->first;
		source_edit && source_edit != source_edits->last;
		source_edit = source_edit->next)
	{
		Asset *dest_asset = edl->assets->update(source_edit->asset);

		Edit *dest_edit = split_edit(postime + source_edit->project_pts, 1);
// Save position of asset
		Asset *split_asset = dest_edit->asset;
		ptstime split_src = dest_edit->source_pts;

		dest_edit->copy_from(source_edit);
		dest_edit->asset = dest_asset;
		dest_edit->project_pts = postime + source_edit->project_pts;

// Shift keyframes in source edit to their position in the
// destination edit for plugin case
		dest_edit->shift_keyframes(postime);

		ptstime req_length = source_edit->length();
		ptstime dst_length = dest_edit->length();

		if(!PTSEQU(req_length, dst_length))
		{
			ptstime end_pos = dest_edit->project_pts + req_length;

			if(req_length < dst_length || dest_edit == last)
			{
				dest_edit = split_edit(end_pos);
				if(dest_edit != last)
				{
					dest_edit->asset = split_asset;
					if(split_asset)
						dest_edit->source_pts = split_src;
					end_pos = dest_edit->next->project_pts + req_length;
				}
			}
			if(dest_edit != last)
			{
				move_edits(dest_edit->next, end_pos,
					MOVE_ALL_EDITS);
			}
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

Edit* Edits::split_edit(ptstime postime, int force)
{
	Edit *edit, *new_edit;
	Transition *trans;

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
	{
		if(force)
		{
			// New edit before the current at the same pts
			new_edit = create_edit();
			trans = edit->transition;
			edit->transition = 0;
			insert_before(edit, new_edit);
			new_edit->copy_from(edit);
			edit->transition = trans;
			return new_edit;
		}
		return edit;
	}
// Split existing edit
	new_edit = create_edit();
	trans = edit->transition;
	edit->transition = 0;

	insert_after(edit, new_edit);
	new_edit->copy_from(edit);
	new_edit->project_pts = postime;
	if(edit->asset)
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
	{
		loaded_length = last->end_pts();
		if(!PTSEQU(loaded_length, project_time))
		{
			Edit *edit = append(create_edit());
			edit->project_pts = loaded_length = project_time;
		}
	}
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
	int edit_mode)
{
	Edit *current_edit;

	for(current_edit = first; current_edit; current_edit = current_edit->next)
	{
		if(edl->equivalent(current_edit->project_pts, oldposition))
		{
			oldposition = current_edit->project_pts;
			break;
		}
	}
	if(current_edit)
	{
		move_edits(current_edit, newposition, edit_mode);
	}
}

void Edits::move_edits(Edit *current_edit, ptstime &newposition, int edit_mode)
{
	Edit *ed;
	ptstime cut_length, apts, oldposition;

	if(current_edit)
	{
		oldposition = current_edit->project_pts;

		if(edit_mode != MOVE_NO_EDITS && newposition < 0)
			newposition = 0;
		if(PTSEQU(oldposition, newposition))
			return;

		if((edit_mode == MOVE_ALL_EDITS || edit_mode == MOVE_ONE_EDIT)
				&& fabs(oldposition) < EPSILON)
			return;
		if(newposition < oldposition)
		{
// shift left
			if((edit_mode == MOVE_NO_EDITS || edit_mode == MOVE_ONE_EDIT)
					&& current_edit->asset)
			{
// Limit shift with the length of asset
				apts = current_edit->project_pts - (current_edit->get_source_end(0)
					- current_edit->source_pts - current_edit->length());
				if(apts > newposition)
					newposition = apts;
			}
			if(edit_mode == MOVE_ALL_EDITS || edit_mode == MOVE_ONE_EDIT)
			{
				cut_length = newposition - oldposition;

				for(ed = current_edit->previous;
						ed && (ed->project_pts > newposition || PTSEQU(ed->project_pts, newposition));
						ed = ed->previous)
					remove(ed);

				current_edit->project_pts = newposition;
				current_edit->shift_keyframes(cut_length);
				if(current_edit->previous)
					current_edit->previous->remove_keyframes_after(newposition);
			}
			else if(edit_mode == MOVE_NO_EDITS)
			{
				if((cut_length = oldposition - newposition) > 0)
					current_edit->source_pts += cut_length;
			}
		}
		else
		{
// shift right
			if(edit_mode == MOVE_ALL_EDITS || edit_mode == MOVE_ONE_EDIT)
			{
				if((ed = current_edit->previous) && ed->asset)
				{
					apts = ed->get_source_end(0) - ed->source_pts + ed->project_pts;

					if(apts < newposition)
						newposition = apts;
				}
				if(edit_mode == MOVE_ONE_EDIT && newposition > current_edit->end_pts())
				{
// Moved over next edit - remove current
					if(current_edit != last)
					{
						newposition = current_edit->end_pts();
						remove(current_edit);
						return;
					}
				}
				current_edit->project_pts = newposition;
				cut_length = newposition - oldposition;
				current_edit->shift_keyframes(cut_length);
				current_edit->remove_keyframes_after(current_edit->end_pts());
			}
			else if(edit_mode == MOVE_NO_EDITS)
			{
				if(current_edit->asset)
				{
					apts = current_edit->source_pts + current_edit->project_pts;

					if(newposition > apts)
						newposition = apts;
				}

				if((cut_length = newposition - oldposition) > 0)
					current_edit->source_pts -= cut_length;
			}
		}

		if(edit_mode == MOVE_ALL_EDITS)
		{
			for(ed = current_edit->next; ed; ed = ed->next)
			{
				ed->project_pts += cut_length;
				ed->shift_keyframes(cut_length);
			}
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

void Edits::dump(int indent)
{
	Edit *edit;

	printf("%*sEdits %p dump(%d):\n", indent, "", this, total());
	for(edit = first; edit; edit = edit->next)
		edit->dump(indent + 2);
}

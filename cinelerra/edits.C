
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

#include "asset.h"
#include "assetlist.h"
#include "automation.h"
#include "bcsignals.h"
#include "clip.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "filesystem.h"
#include "track.h"
#include "transition.h"


#include <string.h>

Edits::Edits(EDL *edl, Track *track)
 : List<Edit>()
{
	this->edl = edl;
	this->track = track;
}

Edit* Edits::create_edit()
{
	return new Edit(edl, this);
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
			ptstime position2 = that_current->get_pts();
			if(*result < 0 || *result > MIN(position1, position2))
				*result = MIN(position1, position2);
			break;
		}
		else
		if(current && !that_current)
		{
			ptstime position1 = edits->length();
			ptstime position2 = current->get_pts();
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
}


Edits& Edits::operator=(Edits& edits)
{
	copy_from(&edits);
	return *this;
}

void Edits::insert_asset(Asset *asset,
	ptstime len_time,
	ptstime postime,
	int track_number,
	int overwrite)
{
	Edit *new_edit, *last_edit, *edit;

	new_edit = split_edit(postime);

	if(overwrite)
	{
		last_edit = split_edit(postime + len_time);
		for(edit = new_edit->next; edit && edit != last_edit;)
		{
			Edit *nxt = edit->next;
			delete edit;
			edit = nxt;
		}
	}
	else
	{
		new_edit = split_edit(postime, 1);
		shift_edits(new_edit->next, len_time);
	}
	new_edit->asset = asset;
	new_edit->set_source_pts(0);

	if(asset->audio_data)
		new_edit->channel = track_number % asset->channels;
	else
	if(asset->video_data)
		new_edit->channel = track_number % asset->layers;
}


void Edits::insert_edits(Edits *source_edits, ptstime postime,
	ptstime duration, int replace)
{
	Edit *first_dest, *last_dest;

	if(duration < 0)
		duration = source_edits->length();

	if(!source_edits->first || fabs(duration) < EPSILON)
		return;

	first_dest = split_edit(postime, 1);
	// do not move first edit
	if(first_dest == first)
		first_dest = first->next;

	if(replace)
	{
		last_dest = split_edit(postime + duration);
		for(Edit *ed = first_dest; ed && ed != last_dest;)
		{
			Edit *nx = ed->next;
			delete ed;
			ed = nx;
		}
	}
	else
	{
		for(Edit *ed = first_dest; ed; ed = ed->next)
			ed->shift(duration);
	}

	for(Edit *source_edit = source_edits->first;
		source_edit && source_edit != source_edits->last &&
			duration > source_edit->get_pts();
		source_edit = source_edit->next)
	{
		if(duration < source_edit->get_pts())
			break;
		Asset *dest_asset = source_edit->asset;
		edl->update_assets(source_edit->asset);

		Edit *dest_edit = split_edit(postime + source_edit->get_pts());

		dest_edit->copy_from(source_edit);
		dest_edit->set_pts(postime + source_edit->get_pts());
	}
}

Edit* Edits::split_edit(ptstime postime, int force)
{
	Edit *edit, *new_edit;
	Transition *trans;

	if(!first)
	{
		// List is empty create edit at 0, next edit at postime
		append(edit = create_edit());
		if(postime > EPSILON || force)
		{
			append(new_edit = create_edit());
			new_edit->set_pts(postime);
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
		edit->set_pts(postime);
		return edit;
	}

	if(PTSEQU(edit->get_pts(), postime))
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
	new_edit->set_pts(postime);
	if(edit->asset)
		new_edit->shift_source(edit->length());

	if(trans)
	{
		if(edit->length() < trans->length_time)
			trans->length_time = edit->length();
		edit->transition = trans;
	}
	return new_edit;
}

// ===================================== file operations

void Edits::load(FileXML *file, int track_offset)
{
	int result = 0;
	ptstime project_time = 0;

	while(last)
		delete last;

	do{
		result = file->read_tag();

		if(!result)
		{
			if(!strcmp(file->tag.get_title(), "EDIT"))
			{
				project_time = load_edit(file, project_time, track_offset);
			}
			else
			if(!strcmp(file->tag.get_title(), "/EDITS"))
			{
				result = 1;
			}
		}
	}while(!result);
}

ptstime Edits::load_edit(FileXML *file, ptstime project_time, int track_offset)
{
	Edit* current;
	ptstime duration;
	int streamno;

	current = append(create_edit());

	duration = current->load_properties(file, project_time);
	if(duration < 0)
		project_time = current->get_pts();
	else
		project_time += duration;

	int result = 0;

	do{
		result = file->read_tag();

		if(!result)
		{
			if(file->tag.title_is("FILE"))
			{
				char filename[BCTEXTLEN];
				char *filepath;

				filename[0] = 0;
				file->tag.get_property("SRC", filename);
				streamno = file->tag.get_property("STREAMNO", -1);
// Extend path
				if(filename[0])
				{
					char directory[BCTEXTLEN];
					FileSystem fs;

					if(filename[0] != '/')
					{
						fs.extract_dir(directory, file->filename);
						strcat(directory, filename);
						filepath = directory;
					}
					else
						filepath = filename;
					current->asset = assetlist_global.get_asset(filepath, streamno);
				}
				else
				{
					current->asset = 0;
				}
			}
			else
			if(file->tag.title_is("TRANSITION"))
			{
				current->transition = new Transition(edl,
					current, 
					"",
					edlsession->default_transition_length);
				current->transition->load_xml(file);
			}
			else
			if(file->tag.title_is("/EDIT"))
			{
				result = 1;
			}
		}
	}while(!result);
	return project_time;
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
		if(PTSEQU(current->get_pts(), postime) ||
				(current->get_pts() <= postime &&
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
		if(current->get_pts() <= postime && 
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

void Edits::save_xml(FileXML *file, const char *output_path)
{
	Edit *current_edit;

	file->tag.set_title("EDITS");
	file->append_tag();
	file->append_newline();

	for(current_edit = first; current_edit; current_edit = current_edit->next)
		current_edit->save_xml(file, output_path, track->data_type);

	file->tag.set_title("/EDITS");
	file->append_tag();
	file->append_newline();
}

void Edits::copy(Edits *edits, ptstime start, ptstime end)
{
	Edit *current_edit, *new_edit = 0;

	for(current_edit = edits->first; current_edit; current_edit = current_edit->next)
	{
		if(current_edit->end_pts() < start - EPSILON)
			continue;

		if(current_edit->get_pts() < start && !PTSEQU(current_edit->get_pts(), start))
		{
			new_edit = append(create_edit());
			new_edit->copy_from(current_edit);
			new_edit->set_pts(0);
			if(new_edit->asset)
			{
				new_edit->set_source_pts(current_edit->get_source_pts() +
					start - current_edit->get_pts());
				if(edl)
					edl->update_assets(new_edit->asset);
			}
		}
		else
		if(current_edit->get_pts() > end && !PTSEQU(current_edit->get_pts(), end))
		{
			new_edit = append(create_edit());
			new_edit->set_pts(end - start);
			break;
		}
		else
		{
			if(fabs(start) > EPSILON)
			{
				new_edit = append(create_edit());
				new_edit->copy_from(current_edit);
				new_edit->shift(-start);
			}
			else
			{
				new_edit = append(create_edit());
				new_edit->copy_from(current_edit);
			}
		}
		if(PTSEQU(current_edit->get_pts(), end))
			break;
	}
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
		edit2->shift_source(end - edit2->get_pts());
		edit2->shift(end - edit2->get_pts());
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
		if(start > edit1->get_source_pts())
			current_edit = split_edit(start);
		else
			current_edit = edit1;
		current_edit->shift_source(len);
		current_edit = current_edit->next;
	}

	for(; current_edit; current_edit = current_edit->next)
		current_edit->shift(-len);
	cleanup();
}

void Edits::clear_after(ptstime pts)
{
	Edit *current;

	if(pts < EPSILON)
		return;

	for(current = first; current;current = NEXT)
	{
		if(current->get_pts() < pts)
			continue;

		if(current->get_pts() > pts)
		{
			current->set_pts(pts);
			break;
		}
	}
	if(current)
	{
		for(current = NEXT; current;)
		{
			Edit *next = NEXT;
			remove(current);
			current = next;
		}
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

			if(current_edit->asset->equivalent(*current_edit->next->asset, 0))
			{

// Got two consecutive edits in same source
				if(edl->equivalent(current_edit->next->get_pts(),
					start))
				{
// handle selected
					ptstime length = -current_edit->length();
					current_edit->next->set_pts(current_edit->next->get_pts() - current_edit->get_source_pts() +
						current_edit->get_pts());
					length += current_edit->length();

// Lengthen automation
					track->automation->paste_silence(current_edit->next->get_pts(),
						current_edit->next->get_pts() + length);

// Lengthen effects
					if(actions & EDIT_PLUGINS)
						track->shift_effects(current_edit->next->get_pts(),
							length);

					for(current_edit = current_edit->next; current_edit; current_edit = current_edit->next)
					{
						current_edit->shift(length);
					}

					distance = length;
					break;
				}
			}
		}
	}
}

void Edits::modify_handles(ptstime oldposition,
	ptstime newposition,
	int edit_handle,
	int edit_mode)
{
	Edit *current_edit;
	ptstime diff, src_pts;

	for(current_edit = first; current_edit; current_edit = current_edit->next)
	{
		if(edl->equivalent(current_edit->get_pts(), oldposition))
			break;
	}

	if(current_edit)
	{
		oldposition = current_edit->get_pts();
// Can't move the first edit
		if((edit_mode == MOVE_ALL_EDITS || edit_mode == MOVE_ONE_EDIT)
				&& current_edit == first)
			return;

		if(PTSEQU(oldposition, newposition))
			return;

		diff = newposition - oldposition;

		if(diff < 0 && (edit_mode == MOVE_ALL_EDITS ||
			edit_mode == MOVE_ONE_EDIT))
		{
			for(Edit *edit = current_edit->previous; edit &&
				edit->get_pts() >= newposition;)
			{
				Edit *tmp = edit->previous;
				remove(edit);
				edit = tmp;
			}
		}
		else if(diff > 0 && edit_mode == MOVE_ONE_EDIT)
		{
			for(Edit *edit = current_edit->next; edit &&
				edit->get_pts() <= newposition;)
			{
				Edit *tmp = edit->next;
				remove(edit);
				edit = tmp;
			}
		}

		switch(edit_mode)
		{
		case MOVE_ALL_EDITS:
			if(edit_handle == HANDLE_LEFT)
				current_edit->shift_source(diff);
			for(; current_edit;current_edit = current_edit->next)
				current_edit->shift(diff);
			break;
		case MOVE_ONE_EDIT:
			current_edit->shift(diff);
			current_edit->shift_source(diff);
			break;
		case MOVE_NO_EDITS:
			current_edit->shift_source(diff);
			break;
		}
	}
}

ptstime Edits::adjust_position(ptstime oldposition, ptstime newposition,
	int edit_handle, int edit_mode)
{
	Edit *edit;

	for(edit = first; edit; edit = edit->next)
	{
		if(edl->equivalent(edit->get_pts(), oldposition))
			break;
	}

	if(edit)
	{
		if(edit_mode != MOVE_NO_EDITS)
		{
			int chk_end = 0;

			if(newposition > oldposition)
				chk_end = 1;

			newposition = limit_move(edit, newposition, 1);
			// Check previohus edit
			if(newposition > oldposition && edit->previous)
			{
				ptstime prevlen = edit->previous->length();
				newposition = limit_source_move(edit->previous,
					newposition - prevlen) + prevlen;
			}
		}

		if(edit_mode == MOVE_ONE_EDIT || edit_mode == MOVE_NO_EDITS ||
				edit_mode == MOVE_ALL_EDITS && edit_handle == HANDLE_LEFT)
			newposition = limit_source_move(edit, newposition);
	}
	return newposition;
}

ptstime Edits::limit_move(Edit *edit, ptstime newposition, int check_end)
{
	if(newposition < 0)
		newposition = 0;

	if(!edit->track->master && newposition > edl->total_length())
		newposition = edl->total_length();

	if(check_end)
	{
		ptstime new_end = newposition + edit->length();
		ptstime tot_len = edl->total_length();

		if(!edit->track->master && new_end > tot_len)
			newposition = tot_len - edit->length();
	}
	return newposition;
}

ptstime Edits::limit_source_move(Edit *edit, ptstime newposition)
{
	if(edit->asset)
	{
		ptstime new_pts = edit->get_source_pts() + newposition - edit->get_pts();
		ptstime src_end = edit->get_source_length();

		if(new_pts + edit->length() > src_end)
		{
			new_pts = src_end - edit->length();
			newposition = new_pts - edit->get_source_pts() + edit->get_pts();
		}

		if(new_pts < 0)
		{
			newposition -= new_pts;
			new_pts = 0;
		}
	}
	return newposition;
}

void Edits::move_edits(Edit *current_edit, ptstime newposition)
{
	Edit *ed, *ted;
	ptstime cut_length, apts, oldposition;

	if(current_edit)
	{
		oldposition = current_edit->get_pts();

		if(newposition < 0)
			newposition = 0;
		if(PTSEQU(oldposition, newposition))
			return;

// Can't move the first edit
		if(current_edit == first)
			return;
		if(newposition < oldposition)
		{
// shift left
			cut_length = newposition - oldposition;

			for(ed = current_edit->previous;
				ed && (ed->get_pts() > newposition || PTSEQU(ed->get_pts(), newposition));
					ed = ted)
			{
				ted = ed->previous;
				remove(ed);
			}

			current_edit->set_pts(newposition);
			current_edit->shift_keyframes(cut_length);
			if(current_edit->previous)
				current_edit->previous->remove_keyframes_after(newposition);
		}
		else
		{
// shift right
			if((ed = current_edit->previous) && ed->asset)
			{
				apts = ed->get_source_length() - ed->get_source_pts() + ed->get_pts();

				if(apts < newposition)
					newposition = apts;
			}
			current_edit->set_pts(newposition);
			cut_length = newposition - oldposition;
			current_edit->shift_keyframes(cut_length);
			current_edit->remove_keyframes_after(current_edit->end_pts());
		}

		for(ed = current_edit->next; ed; ed = ed->next)
		{
			ed->shift(cut_length);
			ed->shift_keyframes(cut_length);
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
		split_edit(start);
		new_edit = split_edit(start, 1);
		new_edit->asset = 0;
		new_edit->set_source_pts(0);
	}
	if(new_edit->next)
		move_edits(new_edit->next, end);
}

void Edits::cleanup()
{
	for(Edit *current = first; current; current = current->next)
	{
		if(!current->asset)
			current->set_source_pts(0);
		if(current == first)
			current->set_pts(0);
		while(current->next)
		{
			if(!current->asset && !current->next->asset)
				remove(current->next);
			else if(current->asset == current->next->asset &&
					PTSEQU(current->get_source_pts() + current->length(), current->next->get_source_pts()))
				remove(current->next);
			else
				break;
		}
	}
	if(first && fabs(first->length()) < EPSILON)
		remove(first);
}

void Edits::shift_edits(Edit *edit, ptstime diff)
{
	if(!edit)
		return;

	if(diff < 0)
	{
		ptstime new_pos = edit->get_pts() + diff;

		if(new_pos < 0)
			return;
// Remove edits what will be overwritten by shifted edits
		for(Edit *ed = first; ed && ed != edit;)
		{
			if(ed->get_pts() > new_pos)
			{
				Edit *next = ed->next;
				delete ed;
				ed = next;
			}
			else
				ed = ed->next;
		}
	}
	else
		if(edit == first)
			edit = edit->next;

	for(; edit; edit = edit->next)
		edit->shift(diff);
}

void Edits::dump(int indent)
{
	Edit *edit;

	printf("%*sEdits %p dump(%d):\n", indent, "", this, total());
	for(edit = first; edit; edit = edit->next)
		edit->dump(indent + 2);
}

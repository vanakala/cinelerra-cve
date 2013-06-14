
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

#include "autos.h"
#include "bcsignals.h"
#include "clip.h"
#include "edl.h"
#include "mainerror.h"
#include "edlsession.h"
#include "localsession.h"
#include "filexml.h"
#include "track.h"

#include <string.h>


Autos::Autos(EDL *edl, Track *track)
 : List<Auto>()
{
	this->edl = edl;
	this->track = track;
	type = -1;
	autoidx = -1;
	autogrouptype = -1;
	base_pts = 0;
}

Autos::~Autos()
{
	while(last) delete last;
}

int Autos::get_type()
{
	return type;
}

Auto* Autos::append_auto()
{
	return append(new_auto());
}

void Autos::equivalent_output(Autos *autos, ptstime startproject, ptstime *result)
{
	for(Auto *current = first, *that_current = autos->first;
		current || that_current;
		current = NEXT,
		that_current = that_current->next)
	{
// Total differs
		if(current && !that_current)
		{
			ptstime position1 = (autos->last ? autos->last->pos_time : startproject);
			ptstime position2 = current->pos_time;
			if(*result < 0 || *result > MIN(position1, position2))
				*result = MIN(position1, position2);
			break;
		}
		else
		if(!current && that_current)
		{
			ptstime position1 = (last ? last->pos_time : startproject);
			ptstime position2 = that_current->pos_time;
			if(*result < 0 || *result > MIN(position1, position2))
				*result = MIN(position1, position2);
			break;
		}
		else
// Keyframes differ
		if(!(*current == *that_current) ||
			!PTSEQU(current->pos_time, that_current->pos_time))
		{
			ptstime position1 = (current->previous ?
				current->previous->pos_time :
				startproject);
			ptstime position2 = (that_current->previous ?
				that_current->previous->pos_time :
				startproject);
			if(*result < 0 || *result > MIN(position1, position2))
				*result = MIN(position1, position2);
			break;
		}
	}
}

void Autos::copy_from(Autos *autos)
{
	Auto *current = autos->first, *this_current = first;

	base_pts = autos->base_pts;

	for(current = autos->first; current; current = NEXT)
	{
		if(!this_current)
		{
			append(this_current = new_auto());
		}
		this_current->copy_from(current);
		this_current = this_current->next;
	}

	for( ; this_current; )
	{
		Auto *next_current = this_current->next;
		delete this_current;
		this_current = next_current;
	}
}

void Autos::insert_track(Autos *automation,
	ptstime start,
	ptstime length)
{
// Insert silence
	insert(start, start + length);
	for(Auto *current = automation->first; current; current = NEXT)
	{
		Auto *new_auto = insert_auto(start + current->pos_time);
		new_auto->copy_from(current);
// Override copy_from
		new_auto->pos_time = current->pos_time + start;
	}
}

Auto* Autos::get_prev_auto(ptstime position,
	Auto* &current)
{
// Get on or before position
// Try existing result
	if(current)
	{
		while(current && current->pos_time < position) 
			current = NEXT;
		while(current && current->pos_time > position) 
			current = PREVIOUS;
	}

	if(!current)
	{
		for(current = last; 
			current && current->pos_time > position; 
			current = PREVIOUS) ;
	}
	if(!current)
		current = first;
	return current;
}

Auto* Autos::get_prev_auto(Auto* &current)
{
	ptstime position = edl->local_session->get_selectionstart(1);

	position = edl->align_to_frame(position, 0);
	return get_prev_auto(position, current);
}

int Autos::auto_exists_for_editing(ptstime position)
{
	int result = 0;

	if(edl->session->auto_keyframes)
	{
		position = edl->align_to_frame(position, 0);
		if(get_auto_at_position(position))
			result = 1;
	}
	else
	{
		result = 1;
	}

	return result;
}

Auto* Autos::get_auto_at_position(ptstime position)
{
	for(Auto *current = first;
		current;
		current = NEXT)
	{
		if(edl->equivalent(current->pos_time, position))
		{
			return current;
		}
	}
	return 0;
}

Auto* Autos::get_auto_for_editing(ptstime position)
{
	Auto *result = 0;

	if(position < 0)
	{
		position = edl->local_session->get_selectionstart(1);
	}
	position = edl->align_to_frame(position, 0);
	if(edl->session->auto_keyframes)
	{
		result = insert_auto(position, 1);
	}
	else
		result = get_prev_auto(position,
			result);

	return result;
}

Auto* Autos::get_next_auto(ptstime position, Auto* &current)
{
	if(current)
	{
		while(current && current->pos_time > position)
			current = PREVIOUS;
		while(current && current->pos_time < position)
			current = NEXT;
	}

	if(!current)
	{
		for(current = first;
			current && current->pos_time <= position;
			current = NEXT);
	}
	if(!current)
		current = last;
	return current;
}

Auto* Autos::insert_auto(ptstime position, int interpolate)
{
	Auto *current, *result;

	position = edl->align_to_frame(position);

	if(position < base_pts)
		return 0;

	if(!first)
	{
		current = append_auto();
		current->pos_time = position;
		return current;
	}
// Test for existence
	for(current = first;
		current && !edl->equivalent(current->pos_time, position);
		current = NEXT);

// Insert new
	if(!current)
	{
// Get first one on or before as a template
		for(current = last;
			current && current->pos_time > position;
			current = PREVIOUS);

		if(current)
		{
			insert_after(current, result = new_auto());
			if(interpolate)
			{
				Auto *next = NEXT;

				result->interpolate_from(current, next, position);
			}
			else
				result->copy_from(current);
			current = result;
			current->pos_time = position;
		}
		else
			current = first;
	}

	return current;
}

Auto* Autos::insert_auto_for_editing(ptstime position)
{
	return insert_auto(position, 1);
}

void Autos::clear_all()
{
	Auto *current_, *current;

	for(current = first; current; current = current_)
	{
		current_ = NEXT;
		remove(current);
	}
	append_auto();
}

void Autos::insert(ptstime start, ptstime end)
{
	ptstime length;
	Auto *current = first;

	if(current)
		current = current->next;

	for( ; current && current->pos_time < start; current = NEXT);

	length = end - start;

	for(; current; current = NEXT)
	{
		current->pos_time += length;
	}
}

void Autos::paste(ptstime start,
	ptstime length,
	double scale,
	FileXML *file)
{
	int total = 0;
	int result = 0;

	do{
		result = file->read_tag();

		if(!result && !file->tag.title_is("/AUTO"))
		{
// End of list
			if(file->tag.get_title()[0] == '/')
			{
				result = 1;
			}
			else
			if(!strcmp(file->tag.get_title(), "AUTO"))
			{
				Auto *current = 0;
				posnum curpos = file->tag.get_property("POSITON", 0);
				ptstime pospts = pos2pts(curpos);
				pospts = file->tag.get_property("POSTIME", pospts) *
						scale + start;
// Paste active auto into track
				current = insert_auto(pospts);

				if(current)
				{
					current->load(file);
				}
				total++;
			}
		}
	}while(!result);
}

void Autos::paste_silence(ptstime start, ptstime end)
{
	insert(start, end);
}

void Autos::copy(ptstime start,
	ptstime end,
	FileXML *file)
{
	for(Auto* current = autoof(start); 
		current && current->pos_time <= end;
		current = NEXT)
	{
// Want to copy single keyframes by putting the cursor on them
		if(current->pos_time >= start && current->pos_time <= end)
			current->copy(start, end, file);
	}
}

void Autos::clear(ptstime start,
	ptstime end,
	int shift_autos)
{
	ptstime length;
	Auto *next, *current;
	length = end - start;

	current = autoof(start);

	if(shift_autos)
	{
		if(base_pts > length)
			base_pts -= length;
		else
			base_pts = 0;
	}
	else
		base_pts = end;

	if(!current)
		return;
// Keep the first
	first->pos_time = base_pts;

	if(current == first)
		current = first->next;

	while(current && current->pos_time < end)
	{
		next = NEXT;
		remove(current);
		current = next;
	}

	while(current && shift_autos)
	{
		current->pos_time -= length;
		current = NEXT;
	}
}

void Autos::load(FileXML *file)
{
	while(last)
		remove(last);    // remove any existing autos

	int result = 0, first_auto = 1;
	Auto *current;

	do{
		result = file->read_tag();

		if(!result && !file->tag.title_is("/AUTO"))
		{
// First tag with leading / is taken as end of autos
			if(file->tag.get_title()[0] == '/')
			{
				result = 1;
			}
			else
			if(!strcmp(file->tag.get_title(), "AUTO"))
			{
				current = append(new_auto());
// Convert from old position
				posnum position = file->tag.get_property("POSITION", (posnum)0);
				current->pos_time = pos2pts(position);
				current->pos_time = file->tag.get_property("POSTIME", current->pos_time);
				current->load(file);
			}
		}
	}while(!result);
}

Auto* Autos::autoof(ptstime position)
{
	Auto *current;

	for(current = first; 
		current && current->pos_time < position; 
		current = NEXT);

	return current;     // return 0 on failure
}

Auto* Autos::nearest_before(ptstime position)
{
	Auto *current;

	for(current = last; 
		current && current->pos_time >= position; current = PREVIOUS);

	return current;     // return 0 on failure
}

Auto* Autos::nearest_after(ptstime position)
{
	Auto *current;

	for(current = first;
		current && current->pos_time <= position; current = NEXT);

	return current;     // return 0 on failure
}

void Autos::get_neighbors(ptstime start, ptstime end,
		Auto **before, Auto **after)
{
	if(*before == 0) *before = first;
	if(*after == 0) *after = last; 

	while(*before && (*before)->next && (*before)->next->pos_time <= start)
		*before = (*before)->next;

	while(*after && (*after)->previous && (*after)->previous->pos_time >= end)
		*after = (*after)->previous;

	while(*before && (*before)->pos_time > start) *before = (*before)->previous;

	while(*after && (*after)->pos_time < end) *after = (*after)->next;
}

void Autos::shift_all(ptstime difference)
{
	Auto *current;

	base_pts += difference;

	for(current = first; current; current = current->next)
		current->pos_time += difference;
}

// Conversions between position and ptstime
ptstime Autos::pos2pts(posnum position)
{
	if(track)
		return track->from_units(position);
	return 0;
}

posnum Autos::pts2pos(ptstime position)
{
	if(track)
		return track->to_units(position, 0);
	return 0;
}

ptstime Autos::unit_round(ptstime pts, int delta)
{
	if(track->data_type == TRACK_VIDEO)
		pts *= edl->session->frame_rate;
	else
		pts *= edl->session->sample_rate;

	pts = round(pts + delta);

	if(track->data_type == TRACK_VIDEO)
		pts /= edl->session->frame_rate;
	else
		pts /= edl->session->sample_rate;
	return pts;
}

void Autos::drag_limits(Auto *current, ptstime *prev, ptstime *next)
{
	if(current->previous)
		*prev = unit_round(current->previous->pos_time, 1);
	else
		*prev = base_pts;

	if(current->next)
		*next = unit_round(current->next->pos_time, -1);
	else
		*next = track->get_length() + base_pts;
}

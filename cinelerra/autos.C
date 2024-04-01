// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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
	while(last)
		delete last;
}

int Autos::get_type()
{
	return type;
}

Auto* Autos::append_auto()
{
	return append(new_auto());
}

ptstime Autos::equivalent_output(Autos *autos, ptstime result)
{
	ptstime pos1, pos2;

	for(Auto *current = first, *that_current = autos->first;
		current || that_current;
		current = NEXT, that_current = that_current->next)
	{
		if(current && !that_current)
		{
			pos1 = (autos->last ? autos->last->pos_time : 0);
			if(pos1 > current->pos_time)
				pos1 = current->pos_time;

			if(result > pos1)
				result = pos1;
			break;
		}
		else if(!current && that_current)
		{
			pos1 = (last ? last->pos_time : 0);
			if(pos1 > that_current->pos_time)
				pos1 = that_current->pos_time;

			if(result > pos1)
				result = pos1;
			break;
		}
		else if(!(*current == *that_current) ||
			!PTSEQU(current->pos_time, that_current->pos_time))
		{
			switch(get_type())
			{
			case AUTOMATION_TYPE_INT:
				pos1 = current->pos_time;
				pos2 = that_current->pos_time;
				break;
			default:
				pos1 = (current->previous ?
					current->previous->pos_time : 0);
				pos2 = (that_current->previous ?
					that_current->previous->pos_time : 0);
				break;
			}
			if(pos1 > pos2)
				pos1 = pos2;

			if(result > pos1)
				result = pos1;
			break;
		}
	}
	return result;
}

void Autos::copy_from(Autos *autos)
{
	Auto *current = autos->first, *this_current = first;

	base_pts = autos->base_pts;
	copy_values(autos);

	for(current = autos->first; current; current = NEXT)
	{
		if(!this_current)
			append(this_current = new_auto());

		this_current->copy_from(current);
		this_current = this_current->next;
	}

	for(; this_current;)
	{
		Auto *next_current = this_current->next;

		delete this_current;
		this_current = next_current;
	}
}

void Autos::insert_track(Autos *automation, ptstime start, ptstime length,
	int overwrite)
{
// Insert silence
	if(overwrite)
		clear(start, start + length, 0);
	else
		insert(start, start + length);

	for(Auto *current = automation->first;
		current && current->pos_time < length;
		current = NEXT)
	{
// fill new auto with values from current (template), interpolate values if possible
		Auto *new_auto = insert_auto(start + current->pos_time, current);
// Override copy_from
		new_auto->pos_time = current->pos_time + start;
	}
}

Auto* Autos::get_prev_auto(ptstime position, Auto* current)
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
		for(current = last; current && current->pos_time > position;
			current = PREVIOUS);
	}

	if(!current)
		current = first;
	return current;
}

Auto* Autos::get_prev_auto(Auto* current)
{
	ptstime position = edl->local_session->get_selectionstart(1);

	position = edl->align_to_frame(position);
	return get_prev_auto(position, current);
}

int Autos::auto_exists_for_editing(ptstime position)
{
	if(edlsession->auto_keyframes)
	{
		position = edl->align_to_frame(position);
		if(get_auto_at_position(position))
			return 1;
	}
	else
		return 1;

	return 0;
}

Auto* Autos::get_auto_at_position(ptstime position)
{
	for(Auto *current = first; current; current = NEXT)
	{
		if(edl->equivalent(current->pos_time, position))
			return current;
	}
	return 0;
}

Auto* Autos::get_auto_for_editing(ptstime position)
{
	Auto *result = 0;

	if(!first)
	{
		result = append_auto();
		result->pos_time = base_pts;
	}

	if(position < 0)
		position = edl->local_session->get_selectionstart(1);

	position = edl->align_to_frame(position);

	if(edlsession->auto_keyframes)
		result = insert_auto(position);
	else
		result = get_prev_auto(position, result);

	return result;
}

Auto* Autos::get_next_auto(ptstime position, Auto* current)
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
		for(current = first; current && current->pos_time <= position;
			current = NEXT);
	}

	if(!current)
		current = last;
	return current;
}

Auto* Autos::insert_auto(ptstime position, Auto *templ)
{
	Auto *current, *result;

	position = edl->align_to_frame(position);

	if(position < base_pts)
		return 0;

	if(!first)
	{
		current = append_auto();
		if(templ)
			current->copy_from(templ);
		current->pos_time = base_pts;
	}
// Test for existence
	for(current = first;
		current && !edl->equivalent(current->pos_time, position);
		current = NEXT);

// Insert new
	if(!current)
	{
// Get first one on or before as a template
		for(current = last; current && current->pos_time > position;
			current = PREVIOUS);

		if(current)
		{
			insert_after(current, result = new_auto());
			current = result;

			if(templ)
				current->copy_from(templ);

			current->pos_time = position;
// interpolate if possible, else copy from template
			result->interpolate_from(0, 0, position, templ);
		}
		else
			current = first;
	}
	return current;
}

void Autos::clear_all()
{
	Auto *next, *current;

	if(first)
	{
		for(current = first->next; current; current = next)
		{
			next = NEXT;
			remove(current);
		}
	}
}

void Autos::insert(ptstime start, ptstime end)
{
	ptstime shift;
	Auto *current = first;

	if(current)
		current = current->next;

	for(; current && current->pos_time < start; current = NEXT);

	shift = end - start;

	for(; current; current = NEXT)
		current->pos_time += shift;
}

void Autos::paste(ptstime start, FileXML *file)
{
	while(!file->read_tag())
	{
		if(!file->tag.title_is("/AUTO"))
		{
// End of list
			if(file->tag.get_title()[0] == '/')
				break;

			if(!strcmp(file->tag.get_title(), "AUTO"))
			{
				Auto *current = 0;
				ptstime pospts = 0;

				if(track)
				{
					posnum curpos = file->tag.get_property("POSITON", 0);
					pospts = track->from_units(curpos);
				}
				pospts = file->tag.get_property("POSTIME", pospts) + start;
// Paste active auto into track
				current = insert_auto(pospts);

				if(current)
					current->load(file);
			}
		}
	}
}

void Autos::paste_silence(ptstime start, ptstime end)
{
	insert(start, end);
}

void Autos::save_xml(FileXML *file)
{
	for(Auto* current = first; current; current = NEXT)
		current->save_xml(file);
}

void Autos::copy(Autos *autos, ptstime start, ptstime end)
{
	Auto *prev_auto = 0;
	Auto *new_auto;

	for(Auto* current = autos->first; current && current->pos_time <= end;
		current = NEXT)
	{
		if(current->pos_time >= start - EPSILON )
		{
			if(prev_auto && current->pos_time > start + EPSILON)
			{
				new_auto = append_auto();
				new_auto->copy_from(prev_auto);
				new_auto->pos_time = 0;
				prev_auto = 0;
			}
			new_auto = append_auto();
			new_auto->copy(current, start, end);
		}
		else
			prev_auto = current;
	}
	// No autos inside selection copy auto before start
	if(!first && autos->first)
	{
		prev_auto = 0;
		prev_auto = autos->get_prev_auto(start, prev_auto);
		new_auto = append_auto();
		new_auto->copy_from(prev_auto);
		new_auto->pos_time = 0;
	}
}

void Autos::clear(ptstime start, ptstime end, int shift_autos)
{
	Auto *next, *current;

	current = autoof(start);

	if(!current)
		return;

// Keep the first
	if(current == first)
		current = first->next;

	while(current && current->pos_time < end)
	{
		next = NEXT;
		remove(current);
		current = next;
	}

	if(shift_autos)
	{
		ptstime length = end - MAX(base_pts, start);
		while(current)
		{
			current->pos_time -= length;
			current = NEXT;
		}
	}
}

void Autos::clear_after(ptstime pts)
{
	Auto *current, *next;

	if(!(current = autoof(pts)))
		return;

	for(current->next; current;)
	{
		next = NEXT;
		remove(current);
		current = next;
	}
}

void Autos::load(FileXML *file)
{
	Auto *current;

	while(last)
		remove(last);    // remove any existing autos

	while(!file->read_tag())
	{
		if(!file->tag.title_is("/AUTO"))
		{
// First tag with leading / is taken as end of autos
			if(file->tag.get_title()[0] == '/')
				break;

			if(!strcmp(file->tag.get_title(), "AUTO"))
			{
				current = append(new_auto());
				if(track)
				{
// Convert from old position
					posnum position = file->tag.get_property("POSITION", (posnum)0);
					current->pos_time = track->from_units(position);
				}
				current->pos_time = file->tag.get_property("POSTIME", current->pos_time);
				current->set_compat_value(compat_value);
				current->load(file);
			}
		}
	}
	compat_value = 0;
}

Auto* Autos::autoof(ptstime position)
{
	Auto *current;

	for(current = first; current && current->pos_time < position;
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
	if(*before == 0)
		*before = first;
	if(*after == 0)
		*after = last;

	while(*before && (*before)->next && (*before)->next->pos_time <= start)
		*before = (*before)->next;

	while(*after && (*after)->previous && (*after)->previous->pos_time >= end)
		*after = (*after)->previous;

	while(*before && (*before)->pos_time > start)
		*before = (*before)->previous;

	while(*after && (*after)->pos_time < end)
		*after = (*after)->next;
}

void Autos::shift_all(ptstime difference)
{
	Auto *current;

	if(first)
		base_pts += difference;

	for(current = first; current; current = current->next)
		current->pos_time += difference;
}

ptstime Autos::unit_round(ptstime pts, int delta)
{
	if(track->data_type == TRACK_VIDEO)
		pts *= edlsession->frame_rate;
	else
		pts *= edlsession->sample_rate;

	pts = round(pts + delta);

	if(track->data_type == TRACK_VIDEO)
		pts /= edlsession->frame_rate;
	else
		pts /= edlsession->sample_rate;
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
		*next = track->duration() + base_pts;
}

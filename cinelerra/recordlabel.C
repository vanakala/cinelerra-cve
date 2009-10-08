
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

#include "clip.h"
#include "labels.h"
#include "recordlabel.h"


RecordLabel::RecordLabel(double position)
 : ListItem<RecordLabel>()
{ 
	this->position = position; 
}

RecordLabel::~RecordLabel()
{
}


RecordLabels::RecordLabels()
 : List<RecordLabel>()
{
}

RecordLabels::~RecordLabels()
{
}

int RecordLabels::delete_new_labels()
{
	RecordLabel *current, *next_;
	for(current = first; current; current = next_)
	{
		next_ = NEXT;
		remove(current);
	} 
	return 0;
}

int RecordLabels::toggle_label(double position)
{
	RecordLabel *current;
// find label the position is after
	for(current = first; 
		current && current->position < position; 
		current = NEXT)
	{
		;
	}

	if(current)
	{
//printf("position %ld current->position %ld current->original %d\n", position, current->position,  current->original);
		if(EQUIV(current->position, position))
		{
// remove it
			remove(current);
		}
		else
		{
// insert before it
			insert_before(current);
			current->position = position;
		}
	}
	else
	{           // insert after last
//printf("position %ld\n", position);
		append(new RecordLabel(position));
	}
	return 0;
}

double RecordLabels::get_prev_label(double position)
{
	RecordLabel *current;
	
	for(current = last; 
		current && current->position > position; 
		current = PREVIOUS)
	{
		;
	}
//printf("%ld\n", current->position);
	if(current && current->position <= position)
		return current->position;
	else
		return -1;
	return 0;
}

double RecordLabels::get_next_label(double position)
{
	RecordLabel *current;

	for(current = first; 
		current && current->position <= position; 
		current = NEXT)
	{
		;
	}
	if(current && current->position >= position) return current->position; else return -1;
	return 0;
}

double RecordLabels::goto_prev_label(double position)
{
	RecordLabel *current;
	
	for(current = last; 
		current && current->position >= position; 
		current = PREVIOUS)
	{
		;
	}
//printf("%ld\n", current->position);
	if(current && current->position <= position) return current->position; else return -1;
	return 0;
}

double RecordLabels::goto_next_label(double position)
{
	RecordLabel *current;

	for(current = first; 
		current && current->position <= position; 
		current = NEXT)
	{
		;
	}
	if(current && current->position >= position) return current->position; else return -1;
	return 0;
}


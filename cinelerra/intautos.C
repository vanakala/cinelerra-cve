
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

#include "automation.inc"
#include "clip.h"
#include "intauto.h"
#include "intautos.h"

IntAutos::IntAutos(EDL *edl, Track *track, int default_value)
 : Autos(edl, track)
{
	this->default_value = default_value;
	type = AUTOMATION_TYPE_INT;
}

Auto* IntAutos::new_auto()
{
	IntAuto *result = new IntAuto(edl, this);
	result->value = default_value;
	return result;
}

int IntAutos::automation_is_constant(ptstime start, ptstime end)
{
	Auto *current_auto, *before = 0, *after = 0;
	int result;

	result = 1;          // default to constant
	if(!last && !first) return result; // no automation at all

// quickly get autos just outside range	
	get_neighbors(start, end, &before, &after);

// autos before range
	if(before) 
		current_auto = before;   // try first auto
	else 
		current_auto = first;

// test autos in range
	for( ; result && 
		current_auto && 
		current_auto->next && 
		current_auto->pos_time < end; 
		current_auto = current_auto->next)
	{
// not constant
		if(((IntAuto*)current_auto->next)->value != ((IntAuto*)current_auto)->value) 
			result = 0;
	}

	return result;
}

int IntAutos::get_automation_constant(ptstime start, ptstime end)
{
	Auto *current_auto, *before = 0, *after = 0;

// quickly get autos just outside range
	get_neighbors(start, end, &before, &after);

// no auto before range so use first
	if(before)
		current_auto = before;
	else
		current_auto = first;

// no autos at all so use default value
	if(!current_auto) return default_value;

	return ((IntAuto*)current_auto)->value;
}

int IntAutos::get_value(ptstime position)
{
	IntAuto *current = (IntAuto *)get_prev_auto(position);

	if(current)
		return current->value;
	return default_value;
}

void IntAutos::get_extents(double *min,
	double *max,
	int *coords_undefined,
	ptstime start,
	ptstime end)
{
	if(!first)
	{
		if(*coords_undefined)
		{
			*min = *max = default_value;
			*coords_undefined = 0;
		}

		*min = MIN(default_value, *min);
		*max = MAX(default_value, *max);
	}

	for(IntAuto *current = (IntAuto*)first; current; current = (IntAuto*)NEXT)
	{
		if(current->pos_time >= start && current->pos_time < end)
		{
			if(*coords_undefined)
			{
				*max = *min = current->value;
				*coords_undefined = 0;
			}
			else
			{
				*min = MIN(current->value, *min);
				*max = MAX(current->value, *max);
			}
		}
	}
}

void IntAutos::copy_values(Autos *autos)
{
	default_value = ((IntAutos*)autos)->default_value;
}

size_t IntAutos::get_size()
{
	size_t size = sizeof(*this);

	for(IntAuto *current = (IntAuto*)first; current; current = (IntAuto*)NEXT)
		size += current->get_size();
	return size;
}

void IntAutos::dump(int indent)
{
	printf("%*sIntautos %p dump(%d): base %.3f default %d\n", indent, "", this, 
		total(), base_pts, default_value);
	indent += 2;
	for(Auto* current = first; current; current = NEXT)
		current->dump(indent);
}

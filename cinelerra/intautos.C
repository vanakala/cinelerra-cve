// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "automation.inc"
#include "clip.h"
#include "intauto.h"
#include "intautos.h"

IntAutos::IntAutos(EDL *edl, Track *track, int default_value)
 : Autos(edl, track)
{
	this->default_value = default_value;
	type = AUTOMATION_TYPE_INT;
	append(new IntAuto(edl,this));
}

Auto* IntAutos::new_auto()
{
	IntAuto *result = new IntAuto(edl, this);
	result->value = default_value;
	return result;
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

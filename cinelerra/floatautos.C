
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
#include "bcsignals.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "mainerror.h"
#include "floatauto.h"
#include "floatautos.h"
#include "track.h"
#include "localsession.h"

FloatAutos::FloatAutos(EDL *edl,
	Track *track,
	float default_value)
 : Autos(edl, track)
{
	this->default_value = default_value;
	type = AUTOMATION_TYPE_FLOAT;
}

void FloatAutos::straighten(ptstime start, ptstime end)
{
	FloatAuto *current = (FloatAuto*)first;

	while(current)
	{
		FloatAuto *previous_auto = (FloatAuto*)PREVIOUS;
		FloatAuto *next_auto = (FloatAuto*)NEXT;

// Is current auto in range?
		if(current->pos_time >= start && current->pos_time < end)
		{
			float current_value = current->value;

// Determine whether to set the control in point.
			if(previous_auto && previous_auto->pos_time >= start)
			{
				float previous_value = previous_auto->value;
				current->control_in_value = (previous_value - current_value) / 6.0;
				if(!current->control_in_pts)
					current->control_in_pts = -1.0;
			}

// Determine whether to set the control out point
			if(next_auto && next_auto->pos_time < end)
			{
				float next_value = next_auto->value;
				current->control_out_value = (next_value - current_value) / 6.0;
				if(!current->control_out_pts)
					current->control_out_pts = 1.0;
			}
		}
		current = (FloatAuto*)NEXT;
	}
}

Auto* FloatAutos::add_auto(ptstime position, float value)
{
	FloatAuto* current = (FloatAuto*)autoof(position);
	FloatAuto* result;

	insert_before(current, result = (FloatAuto*)new_auto());

	result->pos_time = position;
	result->value = value;

	return result;
}

Auto* FloatAutos::new_auto()
{
	FloatAuto *result = new FloatAuto(edl, this);
	result->value = default_value;
	return result;
}

int FloatAutos::automation_is_constant(ptstime start,
	ptstime length,
	double &constant)
{
	int total_autos = total();
	ptstime end;

	end = start + length;

// No keyframes on track
	if(total_autos == 0)
	{
		constant = default_value;
		return 1;
	}
	else
// Only one keyframe on track.
	if(total_autos == 1)
	{
		constant = ((FloatAuto*)first)->value;
		return 1;
	}
	else
// Last keyframe is before region
	if(last->pos_time <= start)
	{
		constant = ((FloatAuto*)last)->value;
		return 1;
	}
	else
// First keyframe is after region
	if(first->pos_time > end)
	{
		constant = ((FloatAuto*)first)->value;
		return 1;
	}

// Scan sequentially
	ptstime prev_position = -1;
	for(Auto *current = first; current; current = NEXT)
	{
		int test_current_next = 0;
		int test_previous_current = 0;
		FloatAuto *float_current = (FloatAuto*)current;

// keyframes before and after region but not in region
		if(prev_position >= 0 &&
			prev_position < start && 
			current->pos_time >= end)
		{
// Get value now in case change doesn't occur
			constant = float_current->value;
			test_previous_current = 1;
		}
		prev_position = current->pos_time;

// Keyframe occurs in the region
		if(!test_previous_current &&
			current->pos_time < end && 
			current->pos_time >= start)
		{

// Get value now in case change doesn't occur
			constant = float_current->value;

// Keyframe has neighbor
			if(current->previous)
				test_previous_current = 1;

			if(current->next)
				test_current_next = 1;
		}

		if(test_current_next)
		{
			FloatAuto *float_next = (FloatAuto*)current->next;

// Change occurs between keyframes
			if(!EQUIV(float_current->value, float_next->value) ||
					!EQUIV(float_current->control_out_value, 0) ||
					!EQUIV(float_next->control_in_value, 0))
				return 0;
		}

		if(test_previous_current)
		{
			FloatAuto *float_previous = (FloatAuto*)current->previous;

// Change occurs between keyframes
			if(!EQUIV(float_current->value, float_previous->value) ||
					!EQUIV(float_current->control_in_value, 0) ||
					!EQUIV(float_previous->control_out_value, 0))
				return 0;
		}
	}

// Got nothing that changes in the region.
	return 1;
}

float FloatAutos::get_value(ptstime position, 
	FloatAuto* &previous,
	FloatAuto* &next)
{
	double slope;
	double intercept;

// Calculate bezier equation at position
	float y0, y1, y2, y3;
	float t;

	previous = (FloatAuto*)get_prev_auto(position, (Auto* &)previous);
	next = (FloatAuto*)get_next_auto(position, (Auto* &)next);

// Constant
	if(!next && !previous)
		return default_value;
	else
	if(!previous)
		return next->value;
	else
	if(!next)
		return previous->value;
	else
	if(next == previous)
		return previous->value;
	else
	{
		if(EQUIV(previous->value, next->value) &&
				EQUIV(previous->control_out_value, 0) &&
				EQUIV(next->control_in_value, 0))
			return previous->value;
	}

// Interpolate
	y0 = previous->value;
	y3 = next->value;

// division by 0
	if(EQUIV(next->pos_time, previous->pos_time))
		return previous->value;

	y1 = previous->value + previous->control_out_value * 2;
	y2 = next->value + next->control_in_value * 2;
	t =  (position - previous->pos_time) / 
		(next->pos_time - previous->pos_time);

	float tpow2 = t * t;
	float tpow3 = t * t * t;
	float invt = 1 - t;
	float invtpow2 = invt * invt;
	float invtpow3 = invt * invt * invt;

	float result = (  invtpow3 * y0
		+ 3 * t     * invtpow2 * y1
		+ 3 * tpow2 * invt     * y2 
		+     tpow3            * y3);

	return result;
}

void FloatAutos::get_extents(float *min, 
	float *max,
	int *coords_undefined,
	ptstime start,
	ptstime end)
{
// Use default auto
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

// Test all handles
	for(FloatAuto *current = (FloatAuto*)first; current; current = (FloatAuto*)NEXT)
	{
		if(current->pos_time >= start && current->pos_time < end)
		{
			if(*coords_undefined)
			{
				*min = *max = current->value;
				*coords_undefined = 0;
			}

			*min = MIN(current->value, *min);
			*min = MIN(current->value + current->control_in_value, *min);
			*min = MIN(current->value + current->control_out_value, *min);

			*max = MAX(current->value, *max);
			*max = MAX(current->value + current->control_in_value, *max);
			*max = MAX(current->value + current->control_out_value, *max);
		}
	}

// Test joining regions
	FloatAuto *prev = 0;
	FloatAuto *next = 0;
	ptstime step = MAX(track->one_unit, edl->local_session->zoom_time);

	for(ptstime position = start;
		position < end;
		position += step)
	{
		float value = get_value(position,
			prev,
			next);
		if(*coords_undefined)
		{
			*min = *max = value;
			*coords_undefined = 0;
		}
		else
		{
			*min = MIN(value, *min);
			*max = MAX(value, *max);
		}
	}
}

void FloatAutos::dump(int ident)
{
	printf("%*sFloatAutos %p dump(%d): base %.3f default %.3f\n", ident, " ", this,
		total(), base_pts, default_value);
	ident += 2;
	for(FloatAuto* current = (FloatAuto*)first; current; current = (FloatAuto*)NEXT)
		current->dump(ident);
}

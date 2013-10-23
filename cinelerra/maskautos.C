
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
#include "maskauto.h"
#include "maskautos.h"


MaskAutos::MaskAutos(EDL *edl, 
	Track *track)
 : Autos(edl, track)
{
	type = AUTOMATION_TYPE_MASK;
	default_mode = MASK_SUBTRACT_ALPHA;
}

void MaskAutos::get_points(ArrayList<MaskPoint*> *points, 
	int submask, ptstime position)
{
	MaskAuto *begin = 0, *end = 0;

// Get auto before and after position
	for(MaskAuto* current = (MaskAuto*)last; 
		current; 
		current = (MaskAuto*)PREVIOUS)
	{
		if(current->pos_time <= position)
		{
			begin = current;
			end = NEXT ? (MaskAuto*)NEXT : current;
			break;
		}
	}

	points->remove_all_objects();
// Nothing before position found
	if(!begin)
		return;

	SubMask *mask1 = begin->get_submask(submask);
	SubMask *mask2 = end->get_submask(submask);

	int total_points = MIN(mask1->points.total, mask2->points.total);

	for(int i = 0; i < total_points; i++)
	{
		MaskPoint *point = new MaskPoint;
		avg_points(point, 
			mask1->points.values[i], 
			mask2->points.values[i],
			position,
			begin->pos_time,
			end->pos_time);
		points->append(point);
	}
}

void MaskAutos::avg_points(MaskPoint *output, 
		MaskPoint *input1, 
		MaskPoint *input2, 
		ptstime output_position,
		ptstime position1,
		ptstime position2)
{
	if(position2 == position1)
		*output = *input1;
	else
	{
		float fraction2 = (output_position - position1) / (position2 - position1);
		float fraction1 = 1 - fraction2;
		output->x = input1->x * fraction1 + input2->x * fraction2;
		output->y = input1->y * fraction1 + input2->y * fraction2;
		output->control_x1 = input1->control_x1 * fraction1 + input2->control_x1 * fraction2;
		output->control_y1 = input1->control_y1 * fraction1 + input2->control_y1 * fraction2;
		output->control_x2 = input1->control_x2 * fraction1 + input2->control_x2 * fraction2;
		output->control_y2 = input1->control_y2 * fraction1 + input2->control_y2 * fraction2;
	}
}

Auto* MaskAutos::new_auto()
{
	return new MaskAuto(edl, this);
}

void MaskAutos::dump(int indent)
{
	printf("%*sMaskAutos %p dump(%d): base %.3f\n", indent, " ", 
		this, total(), base_pts);
	indent += 2;
	for(Auto* current = first; current; current = NEXT)
		((MaskAuto*)current)->dump(indent);
}

int MaskAutos::mask_exists(ptstime position)
{
	Auto *current = 0;

	MaskAuto* keyframe = (MaskAuto*)get_prev_auto(position, current);

	for(int i = 0; i < keyframe->masks.total; i++)
	{
		SubMask *mask = keyframe->get_submask(i);
		if(mask->points.total > 1) 
			return 1;
	}
	return 0;
}

int MaskAutos::total_submasks(ptstime position)
{
	for(MaskAuto* current = (MaskAuto*)last; 
		current; 
		current = (MaskAuto*)PREVIOUS)
	{
		if(current->pos_time <= position)
		{
			return current->masks.total;
		}
	}

	return 0;
}

void MaskAutos::translate_masks(float translate_x, float translate_y)
{
	for(MaskAuto* current = (MaskAuto*)first; 
		current; 
		current = (MaskAuto*)NEXT)
	{
		current->translate_submasks(translate_x, translate_y);
		for(int i = 0; i < current->masks.total; i++)
		{
			SubMask *mask = current->get_submask(i);
			for (int j = 0; j < mask->points.total; j++) 
			{
				mask->points.values[j]->x += translate_x;
				mask->points.values[j]->y += translate_y;
			}
		}
	}
}

int MaskAutos::get_mode()
{
	if(first)
		return ((MaskAuto*)first)->mode;
	else
		default_mode;
}

void MaskAutos::set_mode(int new_mode)
{
	if(first)
		((MaskAuto*)first)->mode = new_mode;
}

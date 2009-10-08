
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
#include "transportque.inc"




MaskAutos::MaskAutos(EDL *edl, 
	Track *track)
 : Autos(edl, track)
{
	type = AUTOMATION_TYPE_MASK;
}

MaskAutos::~MaskAutos()
{
}


void MaskAutos::get_points(ArrayList<MaskPoint*> *points, int submask, int64_t position, int direction)
{
	MaskAuto *begin = 0, *end = 0;
	position = (direction == PLAY_FORWARD) ? position : (position - 1);

// Get auto before and after position
	for(MaskAuto* current = (MaskAuto*)last; 
		current; 
		current = (MaskAuto*)PREVIOUS)
	{
		if(current->position <= position)
		{
			begin = current;
			end = NEXT ? (MaskAuto*)NEXT : current;
			break;
		}
	}

// Nothing before position found
	if(!begin)
	{
		begin = end = (MaskAuto*)first;
	}

// Nothing after position found
	if(!begin)
	{
		begin = end = (MaskAuto*)default_auto;
	}


	SubMask *mask1 = begin->get_submask(submask);
	SubMask *mask2 = end->get_submask(submask);

	points->remove_all_objects();
	int total_points = MIN(mask1->points.total, mask2->points.total);
	for(int i = 0; i < total_points; i++)
	{
		MaskPoint *point = new MaskPoint;
		avg_points(point, 
			mask1->points.values[i], 
			mask2->points.values[i],
			position,
			begin->position,
			end->position);
		points->append(point);
	}
}

void MaskAutos::avg_points(MaskPoint *output, 
		MaskPoint *input1, 
		MaskPoint *input2, 
		int64_t output_position,
		int64_t position1, 
		int64_t position2)
{
	if(position2 == position1)
	{
		*output = *input1;
	}
	else
	{
		float fraction2 = (float)(output_position - position1) / (position2 - position1);
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

void MaskAutos::dump()
{
	printf("	MaskAutos::dump %p\n", this);
	printf("	Default: position %ld submasks %d\n", 
		default_auto->position, 
		((MaskAuto*)default_auto)->masks.total);
	((MaskAuto*)default_auto)->dump();
	for(Auto* current = first; current; current = NEXT)
	{
		printf("	position %ld masks %d\n", 
			current->position, 
			((MaskAuto*)current)->masks.total);
		((MaskAuto*)current)->dump();
	}
}

int MaskAutos::mask_exists(int64_t position, int direction)
{
	Auto *current = 0;
	position = (direction == PLAY_FORWARD) ? position : (position - 1);

	MaskAuto* keyframe = (MaskAuto*)get_prev_auto(position, direction, current);



	for(int i = 0; i < keyframe->masks.total; i++)
	{
		SubMask *mask = keyframe->get_submask(i);
		if(mask->points.total > 1) 
			return 1;
	}
	return 0;
}

int MaskAutos::total_submasks(int64_t position, int direction)
{
	position = (direction == PLAY_FORWARD) ? position : (position - 1);
	for(MaskAuto* current = (MaskAuto*)last; 
		current; 
		current = (MaskAuto*)PREVIOUS)
	{
		if(current->position <= position)
		{
			return current->masks.total;
		}
	}

	return ((MaskAuto*)default_auto)->masks.total;
}


void MaskAutos::translate_masks(float translate_x, float translate_y)
{
	((MaskAuto *)default_auto)->translate_submasks(translate_x, translate_y);
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
				printf("mpx: %f, mpy:%f\n",mask->points.values[j]->x,mask->points.values[j]->y);
			}
		}
		
	}
}


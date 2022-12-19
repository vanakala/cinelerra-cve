// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "automation.inc"
#include "clip.h"
#include "maskauto.h"
#include "maskautos.h"

MaskAutos::MaskAutos(EDL *edl, Track *track)
 : Autos(edl, track)
{
	type = AUTOMATION_TYPE_MASK;
	mode = MASK_SUBTRACT_ALPHA;
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

	if(mask1 && mask2)
	{
		int total_points = MIN(mask1->points.total, mask2->points.total);

		for(int i = 0; i < total_points; i++)
		{
			MaskPoint *point = new MaskPoint;
			avg_points(point,
				mask1->points.values[i],
				mask2->points.values[i],
				position, begin->pos_time, end->pos_time);
			points->append(point);
		}
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
		double fraction2 = (output_position - position1) / (position2 - position1);
		double fraction1 = 1 - fraction2;

		output->submask_x = round(input1->submask_x * fraction1 +
			input2->submask_x * fraction2);
		output->submask_y = round(input1->submask_y * fraction1 +
			input2->submask_y * fraction2);
		output->control_x1 = input1->control_x1 * fraction1 +
			input2->control_x1 * fraction2;
		output->control_y1 = input1->control_y1 * fraction1 +
			input2->control_y1 * fraction2;
		output->control_x2 = input1->control_x2 * fraction1 +
			input2->control_x2 * fraction2;
		output->control_y2 = input1->control_y2 * fraction1 +
			input2->control_y2 * fraction2;
	}
}

Auto* MaskAutos::new_auto()
{
	return new MaskAuto(edl, this);
}

void MaskAutos::copy_values(Autos *autos)
{
	mode = ((MaskAutos*)autos)->mode;
}

size_t MaskAutos::get_size()
{
	size_t size = sizeof(*this);

	for(Auto* current = first; current; current = NEXT)
		size += ((MaskAuto*)current)->get_size();
	return size;
}

void MaskAutos::dump(int indent)
{
	printf("%*sMaskAutos %p dump(%d): base %.3f mode %d\n", indent, " ", 
		this, total(), base_pts, mode);
	indent += 2;
	for(Auto* current = first; current; current = NEXT)
		((MaskAuto*)current)->dump(indent);
}

int MaskAutos::mask_exists(ptstime position)
{
	MaskAuto* keyframe = (MaskAuto*)get_prev_auto(position);

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

void MaskAutos::new_submask()
{
	for(MaskAuto* current = (MaskAuto*)first; current; current = (MaskAuto*)NEXT)
		current->masks.append(new SubMask(current));
}

int MaskAutos::get_mode()
{
	return mode;
}

void MaskAutos::set_mode(int new_mode)
{
	mode = new_mode;
}

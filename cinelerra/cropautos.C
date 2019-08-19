
/*
 * CINELERRA
 * Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>
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
#include "cinelerra.h"
#include "clip.h"
#include "cropauto.h"
#include "cropautos.h"
#include "track.h"

CropAutos::CropAutos(EDL *edl, Track *track)
 : Autos(edl, track)
{
	type = AUTOMATION_TYPE_CROP;
}

Auto* CropAutos::new_auto()
{
	CropAuto *result = new CropAuto(edl, this);
	return result;
}

int CropAutos::automation_is_constant(ptstime start, ptstime end)
{
	Auto *current_auto, *before = 0, *after = 0;

	if(!first)
		return 1; // no automation at all

// quickly get autos just outside range
	get_neighbors(start, end, &current_auto, &after);

// test autos in range
	for(; current_auto && current_auto->next &&
		current_auto->pos_time < end;
		current_auto = current_auto->next)
	{
// not constant
		if(((CropAuto*)current_auto->next)->top != ((CropAuto*)current_auto)->top ||
				((CropAuto*)current_auto->next)->bottom !=
					((CropAuto*)current_auto)->bottom ||
				((CropAuto*)current_auto->next)->left !=
					((CropAuto*)current_auto)->left ||
				((CropAuto*)current_auto->next)->right !=
					((CropAuto*)current_auto)->right)
			return 0;
	}
	return 1;
}

void CropAutos::get_values(ptstime position, int *left, int *right,
	int *top, int *bottom)
{
	CropAuto *before = 0, *after = 0;
	double frac1, frac2;

	if(!first)
	{
		*top = 0;
		*left = 0;
		if(track)
		{
			*right = track->track_w;
			*bottom = track->track_h;
		}
		else
		{
			*right = MAX_FRAME_WIDTH;
			*bottom = MAX_FRAME_HEIGHT;
		}
		return;
	}

	get_neighbors(position, position, (Auto**)&before, (Auto**)&after);

	if(!after || before == after)
	{
		*left = before->left;
		*right = before->right;
		*top = before->top;
		*bottom = before->bottom;
		return;
	}
	frac1 = (position - before->pos_time) /
		(after->pos_time - before->pos_time);
	frac2 = 1.0 - frac1;

	*top = round(before->top * frac2 + after->top * frac1);
	*left = round(before->left * frac2 + after->left * frac1);
	*right = round(before->right * frac2 + after->right * frac1);
	*bottom = round(before->bottom * frac2 + after->bottom * frac1);
}

size_t CropAutos::get_size()
{
	size_t size = sizeof(*this);

	for(CropAuto *current = (CropAuto*)first; current; current = (CropAuto*)NEXT)
		size += current->get_size();
	return size;
}

void CropAutos::dump(int indent)
{
	printf("%*sCropAutos %p dump(%d): base %.3f\n", indent, "", this,
		total(), base_pts);
	indent += 2;
	for(Auto* current = first; current; current = NEXT)
		current->dump(indent);
}

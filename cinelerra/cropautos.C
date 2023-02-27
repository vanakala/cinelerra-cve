// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>

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
	append(new CropAuto(this));
}

Auto* CropAutos::new_auto()
{
	return new CropAuto(this);
}

CropAuto *CropAutos::get_values(ptstime position, int *left, int *right,
	int *top, int *bottom)
{
	CropAuto *cropauto;
	double left_f, right_f, bottom_f, top_f;

	cropauto = get_values(position, &left_f, &right_f, &top_f, &bottom_f);

	*left = round(left_f * track->track_w);
	*right = round(right_f * track->track_w);
	*top = round(top_f * track->track_h);
	*bottom = round(bottom_f * track->track_h);
	return cropauto;
}

CropAuto *CropAutos::get_values(ptstime position, double *left, double *right,
	double *top, double *bottom)
{
	CropAuto *before = 0, *after = 0;
	double frac1, frac2;

	if(!first)
	{
		*top = 0;
		*left = 0;
		*right = 1.0;
		*bottom = 1.0;
		return 0;
	}

	get_neighbors(position, position, (Auto**)&before, (Auto**)&after);

	if(!after || before == after)
	{
		*left = before->left;
		*right = before->right;
		*top = before->top;
		*bottom = before->bottom;
		return before;
	}
	frac1 = (position - before->pos_time) /
		(after->pos_time - before->pos_time);
	frac2 = 1.0 - frac1;

	*top = before->top * frac2 + after->top * frac1;
	*left = before->left * frac2 + after->left * frac1;
	*right = before->right * frac2 + after->right * frac1;
	*bottom = before->bottom * frac2 + after->bottom * frac1;
	return before;
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

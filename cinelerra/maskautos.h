
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

#ifndef MASKAUTOS_H
#define MASKAUTOS_H


#include "autos.h"
#include "edl.inc"
#include "maskauto.inc"
#include "track.inc"

class MaskAutos : public Autos
{
public:
	MaskAutos(EDL *edl, Track *track);
	~MaskAutos();

	Auto* new_auto();


	void dump();

	static void avg_points(MaskPoint *output, 
		MaskPoint *input1, 
		MaskPoint *input2, 
		ptstime output_position,
		ptstime position1, 
		ptstime position2);
	int mask_exists(ptstime position, int direction);
// Perform interpolation
	void get_points(ArrayList<MaskPoint*> *points, int submask, ptstime position);
	int total_submasks(ptstime position);
// Translates all mask points
	void translate_masks(float translate_x, float translate_y);
};




#endif

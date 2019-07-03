
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

#ifndef FLOATAUTOS_H
#define FLOATAUTOS_H

#include "autos.h"
#include "edl.inc"
#include "filexml.inc"
#include "floatauto.inc"

class FloatAutos : public Autos
{
public:
	FloatAutos(EDL *edl, 
		Track *track,
// Value for default auto
		float default_value);

// Return 1 if the automation is constant.
// constant - set to the value if it is constant
	int automation_is_constant(ptstime start,
		ptstime length,
		double &constant);

// Get value at a specific point.  This needs previous and next stores
// because it is used for every pixel in the drawing function.
	float get_value(ptstime position,
		FloatAuto* &previous,
		FloatAuto* &next);

// Helper: just calc the bezier function without doing any lookup of nodes
	static float calculate_bezier(FloatAuto *previous, FloatAuto *next,
		ptstime position);
	static float calculate_bezier_derivation(FloatAuto *previous,
		FloatAuto *next, ptstime position);

	void get_extents(float *min, 
		float *max,
		int *coords_undefined,
		ptstime start,
		ptstime end);
	void straighten(ptstime start, ptstime end);

	Auto* append_auto();
	Auto* new_auto();
	void copy_values(Autos *autos);
	size_t get_size();
	void dump(int ident = 0);

	float default_value;
};

#endif

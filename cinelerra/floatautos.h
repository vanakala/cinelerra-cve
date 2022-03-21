// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FLOATAUTOS_H
#define FLOATAUTOS_H

#include "autos.h"
#include "edl.inc"
#include "filexml.inc"
#include "floatauto.inc"

class FloatAutos : public Autos
{
public:
	FloatAutos(EDL *edl, Track *track, float default_value);

// Return 1 if the automation is constant.
// constant - set to the value if it is constant
	int automation_is_constant(ptstime start,
		ptstime length,
		double *constant);

// Get value at a specific point.
	float get_value(ptstime position,
		FloatAuto* previous = 0,
		FloatAuto* next = 0);

// Helper: just calc the bezier function without doing any lookup of nodes
	static float calculate_bezier(FloatAuto *previous, FloatAuto *next,
		ptstime position);
	static float calculate_bezier_derivation(FloatAuto *previous,
		FloatAuto *next, ptstime position);

	void get_extents(double *min,
		double *max,
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

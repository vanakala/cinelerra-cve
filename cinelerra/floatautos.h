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
	FloatAutos(EDL *edl, Track *track, double default_value);

// Return 1 if the automation is constant.
// constant - set to the value if it is constant
	int automation_is_constant(ptstime start,
		ptstime length,
		double *constant);

// Get value at a specific point.
	double get_value(ptstime position, FloatAuto* previous = 0,
		FloatAuto* next = 0);

// Get raw value at a specific point.
	double get_raw_value(ptstime position, FloatAuto* previous = 0,
		FloatAuto* next = 0);

// Helper: just calc the bezier function without doing any lookup of nodes
	static double calculate_bezier(FloatAuto *previous, FloatAuto *next,
		ptstime position);
	static double calculate_bezier_derivation(FloatAuto *previous,
		FloatAuto *next, ptstime position);

	void get_extents(double *min,
		double *max,
		int *coords_undefined,
		ptstime start,
		ptstime end);
	void straighten(ptstime start, ptstime end);
	double scale_value(double value);

	Auto* append_auto();
	Auto* new_auto();
	void copy_values(Autos *autos);
	size_t get_size();
	void dump(int ident = 0);

	double default_value;
};

#endif

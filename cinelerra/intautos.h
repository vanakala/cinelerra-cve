// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef INTAUTOS_H
#define INTAUTOS_H

#include "autos.h"
#include "edl.inc"
#include "track.inc"

class IntAutos : public Autos
{
public:
	IntAutos(EDL *edl, Track *track, int default_value);

	Auto* new_auto();
	int automation_is_constant(ptstime start, ptstime end);
	int get_automation_constant(ptstime start, ptstime end);
	void get_extents(double *min,
		double *max,
		int *coords_undefined,
		ptstime start,
		ptstime end);
	int get_value(ptstime position);
	void copy_values(Autos *autos);
	size_t get_size();
	void dump(int indent = 0);

	int default_value;
};

#endif


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
	void get_extents(float *min, 
		float *max,
		int *coords_undefined,
		ptstime start,
		ptstime end);
	int get_value(ptstime position);
	void dump(int indent = 0);

	int default_value;
};

#endif

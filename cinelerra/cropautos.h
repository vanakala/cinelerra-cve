
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

#ifndef CROPAUTOS_H
#define CROPAUTOS_H

#include "autos.h"
#include "cropauto.inc"
#include "cropautos.inc"
#include "edl.inc"
#include "track.inc"

class CropAutos : public Autos
{
public:
	CropAutos(EDL *edl, Track *track);

	Auto* new_auto();
	int automation_is_constant(ptstime start, ptstime end);
	CropAuto *get_values(ptstime position, int *left, int *right,
		int *top, int *bottom);
	size_t get_size();
	void dump(int indent = 0);
};

#endif

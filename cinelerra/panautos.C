
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

#include "automation.inc"
#include "panauto.h"
#include "panautos.h"
#include "transportque.inc"

PanAutos::PanAutos(EDL *edl, Track *track)
 : Autos(edl, track)
{
	type = AUTOMATION_TYPE_PAN;
}

PanAutos::~PanAutos()
{
}


Auto* PanAutos::new_auto()
{
	return new PanAuto(edl, this);
}

void PanAutos::get_handle(int &handle_x,
		int &handle_y,
		int64_t position, 
		int direction,
		PanAuto* &previous,
		PanAuto* &next)
{
	previous = (PanAuto*)get_prev_auto(position, direction, (Auto* &)previous);
	next = (PanAuto*)get_next_auto(position, direction, (Auto* &)next);

// Constant
	if(previous->handle_x == next->handle_x &&
		previous->handle_y == next->handle_y)
	{
		handle_x = previous->handle_x;
		handle_y = previous->handle_y;
		return;
	}

// Interpolate
	int64_t total = labs(next->position - previous->position);
	double fraction;
	if(direction == PLAY_FORWARD)
	{
		fraction = (double)(position - previous->position) / total;
	}
	else
	{
		fraction = (double)(previous->position - position) / total;
	}

	handle_x = (int)(previous->handle_x + (next->handle_x - previous->handle_x) * fraction);
	handle_y = (int)(previous->handle_y + (next->handle_y - previous->handle_y) * fraction);
}

void PanAutos::dump()
{
	printf("	PanAutos::dump %p\n", this);
		printf("	Default: position %ld\n", default_auto->position);
		((PanAuto*)default_auto)->dump();
	for(Auto* current = first; current; current = NEXT)
	{
		printf("	position %ld\n", current->position);
		((PanAuto*)current)->dump();
	}
}

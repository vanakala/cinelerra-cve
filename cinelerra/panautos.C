
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
#include "edl.h"
#include "edlsession.h"
#include "panauto.h"
#include "panautos.h"

#include <string.h>

PanAutos::PanAutos(EDL *edl, Track *track)
 : Autos(edl, track)
{
	type = AUTOMATION_TYPE_PAN;
	memset(default_values, 0, MAXCHANNELS * sizeof(float));
	default_handle_x = 0;
	default_handle_y = 0;
}

Auto* PanAutos::new_auto()
{
	PanAuto* r = new PanAuto(edl, this);
	memcpy(r->values, default_values, MAXCHANNELS * sizeof(float));
	r->handle_x = default_handle_x;
	r->handle_y = default_handle_y;
	return r;
}

void PanAutos::get_handle(int &handle_x,
		int &handle_y,
		ptstime position,
		PanAuto* &previous,
		PanAuto* &next)
{
	if(!first)
	{
		previous = 0;
		next = 0;
		handle_x = default_handle_x;
		handle_y = default_handle_y;
		return;
	}
	previous = (PanAuto*)get_prev_auto(position, (Auto* &)previous);
	next = (PanAuto*)get_next_auto(position, (Auto* &)next);

// Constant
	if(previous->handle_x == next->handle_x &&
		previous->handle_y == next->handle_y)
	{
		handle_x = previous->handle_x;
		handle_y = previous->handle_y;
		return;
	}

// Interpolate
	ptstime total = fabs(next->pos_time - previous->pos_time);
	double fraction;
	fraction = (position - previous->pos_time) / total;

	handle_x = (int)(previous->handle_x + (next->handle_x - previous->handle_x) * fraction);
	handle_y = (int)(previous->handle_y + (next->handle_y - previous->handle_y) * fraction);
}

void PanAutos::dump(int indent)
{
	printf("%*sPanAutos %p dump(%d): base %.3f handles: %d, %d\n", indent, " ", this, 
		total(), base_pts, default_handle_x, default_handle_y);
	printf("%*sdefault values:", indent + 4, " ");
	if(edl)
	{
		for(int i = 0; i < edl->session->audio_channels; i++)
			printf(" %.1f", default_values[i]);
		putchar('\n');
	}
	indent += 2;
	for(Auto* current = first; current; current = NEXT)
		((PanAuto*)current)->dump(indent);
}

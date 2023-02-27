// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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
	memset(default_values, 0, MAXCHANNELS * sizeof(double));
	default_handle_x = 0;
	default_handle_y = 0;
}

Auto* PanAutos::new_auto()
{
	PanAuto* r = new PanAuto(this);
	memcpy(r->values, default_values, MAXCHANNELS * sizeof(double));
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
	previous = (PanAuto*)get_prev_auto(position, (Auto*)previous);
	next = (PanAuto*)get_next_auto(position, (Auto*)next);

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

void PanAutos::copy_values(Autos *autos)
{
	PanAutos *pas = (PanAutos*)autos;

	default_handle_x = pas->default_handle_x;
	default_handle_y = pas->default_handle_y;
	memcpy(default_values, pas->default_values, sizeof(default_values));
}

size_t PanAutos::get_size()
{
	size_t size = sizeof(*this);

	for(Auto* current = first; current; current = NEXT)
		size += ((PanAuto*)current)->get_size();
	return size;
}

void PanAutos::dump(int indent)
{
	printf("%*sPanAutos %p dump(%d): base %.3f handles: %d, %d\n", indent, " ", this, 
		total(), base_pts, default_handle_x, default_handle_y);
	if(edl)
	{
		printf("%*sdefault values:", indent + 4, " ");
		for(int i = 0; i < edlsession->audio_channels; i++)
			printf(" %.1f", default_values[i]);
		putchar('\n');
	}
	indent += 2;
	for(Auto* current = first; current; current = NEXT)
		((PanAuto*)current)->dump(indent);
}

// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PANAUTOS_H
#define PANAUTOS_H

#include "autos.h"
#include "cinelerra.h"
#include "edl.inc"
#include "panauto.inc"
#include "track.inc"

class PanAutos : public Autos
{
public:
	PanAutos(EDL *edl, Track *track);

	Auto* new_auto();
	void get_handle(int &handle_x,
		int &handle_y,
		ptstime position,
		PanAuto* &previous,
		PanAuto* &next);
	void copy_values(Autos *autos);
	size_t get_size();
	void dump(int indent = 0);

	double default_values[MAXCHANNELS];
	int default_handle_x;
	int default_handle_y;
};

#endif

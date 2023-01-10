// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>

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
	CropAuto *get_values(ptstime position, int *left, int *right,
		int *top, int *bottom);
	CropAuto *get_values(ptstime position, double *left, double *right,
		double *top, double *bottom);
	size_t get_size();
	void dump(int indent = 0);
};

#endif

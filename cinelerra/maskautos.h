// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef MASKAUTOS_H
#define MASKAUTOS_H

#include "autos.h"
#include "edl.inc"
#include "maskauto.inc"
#include "track.inc"

class MaskAutos : public Autos
{
public:
	MaskAutos(EDL *edl, Track *track);

	Auto* new_auto();

	void dump(int indent = 0);

	static void avg_points(MaskPoint *output, 
		MaskPoint *input1, 
		MaskPoint *input2, 
		ptstime output_position,
		ptstime position1, 
		ptstime position2);
	int mask_exists(ptstime position);
// Perform interpolation
	void get_points(ArrayList<MaskPoint*> *points, int submask, ptstime position);
	int total_submasks(ptstime position);
	void new_submask();
	void copy_values(Autos *autos);

	int get_mode();
	void set_mode(int new_mode);
	size_t get_size();
private:
	int mode;
};

#endif

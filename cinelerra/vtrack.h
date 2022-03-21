// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef VTRACK_H
#define VTRACK_H

#include "edl.inc"
#include "filexml.inc"
#include "track.h"
#include "vframe.inc"


class VTrack : public Track
{
public:
	VTrack(EDL *edl, Tracks *tracks);

	void set_default_title();
	void save_header(FileXML *file);
	posnum to_units(ptstime position, int round = 0);
	ptstime from_units(posnum position);

	void calculate_output_transfer(ptstime position,
		int *in_x, int *in_y, int *in_w, int *in_h,
		int *out_x, int *out_y, int *out_w, int *out_h);

	int vertical_span(Theme *theme);
	void translate(float offset_x, float offset_y, int do_camera);

};

#endif

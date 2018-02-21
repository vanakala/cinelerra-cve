
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

#ifndef VTRACK_H
#define VTRACK_H

#include "arraylist.h"
#include "autoconf.inc"
#include "edl.inc"
#include "filexml.inc"
#include "floatautos.inc"
#include "linklist.h"
#include "track.h"
#include "vedit.inc"
#include "vframe.inc"


class VTrack : public Track
{
public:
	VTrack(EDL *edl, Tracks *tracks);

	void set_default_title();
	void save_header(FileXML *file);
	void copy_settings(Track *track);
	void synchronize_params(Track *track);
	posnum to_units(ptstime position, int round = 0);
	ptstime from_units(posnum position);

	void calculate_input_transfer(Asset *asset, ptstime position,
		int *in_x, int *in_y, int *in_w, int *in_h,
		int *out_x, int *out_y, int *out_w, int *out_h);

	void calculate_output_transfer(ptstime position,
		int *in_x, int *in_y, int *in_w, int *in_h,
		int *out_x, int *out_y, int *out_w, int *out_h);

	int vertical_span(Theme *theme);
	void translate(float offset_x, float offset_y, int do_camera);

};

#endif

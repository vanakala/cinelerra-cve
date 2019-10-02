
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * Copyright (C) 2013 Einar Rünkaru <einarrunkaru at gmail dot com>
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

#ifndef LEVELHIST_H
#define LEVELHIST_H

#include "aframe.inc"
#include "cinelerra.h"
#include "datatype.h"
#include "levelhist.inc"

class LevelHistory
{
public:
	LevelHistory();
	~LevelHistory();

	void reset(int fragment_len, int samplerate, int channels);
	void fill(AFrame **frames);
	int get_levels(double *levels, ptstime pts);

private:
	int meter_render_fragment;
	int total_peaks;
	int channels;
	int rate;
	ptstime *level_pts;
	int current_peak;
	double *level_history[MAXCHANNELS];
};

#endif

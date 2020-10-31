// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
// Copyright (C) 2013 Einar Rünkaru <einarrunkaru at gmail dot com>

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
	void clear();
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

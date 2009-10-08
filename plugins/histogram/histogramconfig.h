
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

#ifndef HISTOGRAMCONFIG_H
#define HISTOGRAMCONFIG_H


#include "histogram.inc"
#include "histogramconfig.inc"
#include "linklist.h"
#include <stdint.h>

class HistogramPoint : public ListItem<HistogramPoint>
{
public:
	HistogramPoint();
	~HistogramPoint();

	int equivalent(HistogramPoint *src);
	float x, y;
};


class HistogramPoints : public List<HistogramPoint>
{
public:
	HistogramPoints();
	~HistogramPoints();

// Insert new point
	HistogramPoint* insert(float x, float y);
	int equivalent(HistogramPoints *src);
	void boundaries();
	void copy_from(HistogramPoints *src);
	void interpolate(HistogramPoints *prev, 
		HistogramPoints *next,
		double prev_scale,
		double next_scale);
};

class HistogramConfig
{
public:
	HistogramConfig();

	int equivalent(HistogramConfig &that);
	void copy_from(HistogramConfig &that);
	void interpolate(HistogramConfig &prev, 
		HistogramConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
// Used by constructor and reset button
	void reset(int do_mode);
	void reset_points(int colors_only);
	void boundaries();
	void dump();

// Range 0 - 1.0
// Input points
	HistogramPoints points[HISTOGRAM_MODES];
// Output points
	float output_min[HISTOGRAM_MODES];
	float output_max[HISTOGRAM_MODES];
	int automatic;
	float threshold;
	int plot;
	int split;
};


#endif




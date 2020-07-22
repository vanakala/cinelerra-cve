// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef HISTOGRAMCONFIG_H
#define HISTOGRAMCONFIG_H

#include "datatype.h"
#include "linklist.h"

class HistogramPoint : public ListItem<HistogramPoint>
{
public:
	HistogramPoint();

	int equivalent(HistogramPoint *src);
	double x, y;
};


class HistogramPoints : public List<HistogramPoint>
{
public:
	HistogramPoints();

// Insert new point
	HistogramPoint* insert(double x, double y);
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
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);
// Used by constructor and reset button
	void reset(int do_mode);
	void reset_points(int colors_only);
	void boundaries();
	void dump(int indent = 0);

// Range 0 - 1.0
// Input points
	HistogramPoints points[HISTOGRAM_MODES];
// Output points
	double output_min[HISTOGRAM_MODES];
	double output_max[HISTOGRAM_MODES];
	int automatic;
	double threshold;
	int plot;
	int split;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

#endif

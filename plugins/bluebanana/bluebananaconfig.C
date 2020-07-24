// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2012-2013 Monty <monty@xiph.org>
// Cinelerra :: Blue Banana - color modification plugin for Cinelerra-CV

#include "bluebanana.h"
#include "units.h"

#include <math.h>

BluebananaConfig::BluebananaConfig()
{
	mark = 0;
	active = 1;
	use_mask = 0;
	capture_mask = 1; // has no effect if use_mask is off
	invert_selection = 0;

	Hsel_active = 1;
	Hsel_lo = 0;
	Hsel_hi = 360;
	Hsel_over = 36;

	Ssel_active = 1;
	Ssel_lo = 0;
	Ssel_hi = 100;
	Ssel_over = 10;

	Vsel_active = 1;
	Vsel_lo = 0;
	Vsel_hi = 100;
	Vsel_over = 10;

	Fsel_active = 0;
	Fsel_erode = 0;
	Fsel_lo = -2;
	Fsel_mid = 0;
	Fsel_hi = 2;
	Fsel_over = 5;

	Hadj_active = 0;
	Hadj_val = 0;

	Sadj_active = 0;
	Sadj_lo = 0;
	Sadj_gamma = 1;
	Sadj_hi = 100;

	Vadj_active = 0;
	Vadj_lo = 0;
	Vadj_gamma = 1;
	Vadj_hi = 100;

	Radj_active = 0;
	Radj_lo = 0;
	Radj_gamma = 1;
	Radj_hi = 100;

	Gadj_active = 0;
	Gadj_lo = 0;
	Gadj_gamma = 1;
	Gadj_hi = 100;

	Badj_active = 0;
	Badj_lo = 0;
	Badj_gamma = 1;
	Badj_hi = 100;

	Oadj_active = 0;
	Oadj_val = 100;
}

int BluebananaConfig::equivalent(BluebananaConfig &that)
{
	if(active != that.active) return 0;
	if(mark != that.mark) return 0;
	if(use_mask != that.use_mask) return 0;
	if(capture_mask != that.capture_mask) return 0;
	if(invert_selection != that.invert_selection) return 0;

	if(Hsel_active != that.Hsel_active) return 0;
	if(Hsel_lo != that.Hsel_lo) return 0;
	if(Hsel_hi != that.Hsel_hi) return 0;
	if(Hsel_over != that.Hsel_over) return 0;

	if(Ssel_active != that.Ssel_active) return 0;
	if(Ssel_lo != that.Ssel_lo) return 0;
	if(Ssel_hi != that.Ssel_hi) return 0;
	if(Ssel_over != that.Ssel_over) return 0;

	if(Vsel_active != that.Vsel_active) return 0;
	if(Vsel_lo != that.Vsel_lo) return 0;
	if(Vsel_hi != that.Vsel_hi) return 0;
	if(Vsel_over != that.Vsel_over) return 0;

	if(Fsel_active != that.Fsel_active) return 0;
	if(Fsel_erode != that.Fsel_erode) return 0;
	if(Fsel_lo != that.Fsel_lo) return 0;
	if(Fsel_mid != that.Fsel_mid) return 0;
	if(Fsel_hi != that.Fsel_hi) return 0;
	if(Fsel_over != that.Fsel_over) return 0;

	if(Hadj_active != that.Hadj_active) return 0;
	if(Hadj_val != that.Hadj_val) return 0;

	if(Oadj_active != that.Oadj_active) return 0;
	if(Oadj_val != that.Oadj_val) return 0;

	if(Sadj_active != that.Sadj_active) return 0;
	if(Sadj_gamma != that.Sadj_gamma) return 0;
	if(Sadj_lo != that.Sadj_lo) return 0;
	if(Sadj_hi != that.Sadj_hi) return 0;

	if(Vadj_active != that.Vadj_active) return 0;
	if(Vadj_gamma != that.Vadj_gamma) return 0;
	if(Vadj_lo != that.Vadj_lo) return 0;
	if(Vadj_hi != that.Vadj_hi) return 0;

	if(Radj_active != that.Radj_active) return 0;
	if(Radj_gamma != that.Radj_gamma) return 0;
	if(Radj_lo != that.Radj_lo) return 0;
	if(Radj_hi != that.Radj_hi) return 0;

	if(Gadj_active != that.Gadj_active) return 0;
	if(Gadj_gamma != that.Gadj_gamma) return 0;
	if(Gadj_lo != that.Gadj_lo) return 0;
	if(Gadj_hi != that.Gadj_hi) return 0;

	if(Badj_active != that.Badj_active) return 0;
	if(Badj_gamma != that.Badj_gamma) return 0;
	if(Badj_lo != that.Badj_lo) return 0;
	if(Badj_hi != that.Badj_hi) return 0;

	return 1;
}

void BluebananaConfig::copy_from(BluebananaConfig &that)
{
	mark = that.mark;
	active = that.active;
	use_mask = that.use_mask;
	capture_mask = that.capture_mask;
	invert_selection = that.invert_selection;

	Hsel_active = that.Hsel_active;
	Hsel_lo = that.Hsel_lo;
	Hsel_hi = that.Hsel_hi;
	Hsel_over = that.Hsel_over;
	Ssel_active = that.Ssel_active;
	Ssel_lo = that.Ssel_lo;
	Ssel_hi = that.Ssel_hi;
	Ssel_over = that.Ssel_over;
	Vsel_active = that.Vsel_active;
	Vsel_lo = that.Vsel_lo;
	Vsel_hi = that.Vsel_hi;
	Vsel_over = that.Vsel_over;
	Fsel_active = that.Fsel_active;
	Fsel_erode = that.Fsel_erode;
	Fsel_lo = that.Fsel_lo;
	Fsel_mid = that.Fsel_mid;
	Fsel_hi = that.Fsel_hi;
	Fsel_over = that.Fsel_over;

	Hadj_active = that.Hadj_active;
	Hadj_val = that.Hadj_val;

	Sadj_active = that.Sadj_active;
	Sadj_gamma = that.Sadj_gamma;
	Sadj_lo = that.Sadj_lo;
	Sadj_hi = that.Sadj_hi;

	Vadj_active = that.Vadj_active;
	Vadj_gamma = that.Vadj_gamma;
	Vadj_lo = that.Vadj_lo;
	Vadj_hi = that.Vadj_hi;

	Radj_active = that.Radj_active;
	Radj_gamma = that.Radj_gamma;
	Radj_lo = that.Radj_lo;
	Radj_hi = that.Radj_hi;

	Gadj_active = that.Gadj_active;
	Gadj_gamma = that.Gadj_gamma;
	Gadj_lo = that.Gadj_lo;
	Gadj_hi = that.Gadj_hi;

	Badj_active = that.Badj_active;
	Badj_gamma = that.Badj_gamma;
	Badj_lo = that.Badj_lo;
	Badj_hi = that.Badj_hi;

	Oadj_active = that.Oadj_active;
	Oadj_val = that.Oadj_val;
}

void BluebananaConfig::interpolate(BluebananaConfig &prev,
	BluebananaConfig &next,
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	active = prev.active;
	use_mask = prev.use_mask;
	capture_mask = prev.capture_mask;
	invert_selection = prev.invert_selection;

	Hsel_active = prev.Hsel_active;
	Ssel_active = prev.Ssel_active;
	Vsel_active = prev.Vsel_active;
	Fsel_active = prev.Fsel_active;
	Fsel_erode = prev.Fsel_erode;
	Hadj_active = prev.Hadj_active;
	Sadj_active = prev.Sadj_active;
	Vadj_active = prev.Vadj_active;
	Radj_active = prev.Radj_active;
	Gadj_active = prev.Gadj_active;
	Badj_active = prev.Badj_active;
	Oadj_active = prev.Oadj_active;

	Hsel_lo = prev.Hsel_lo * prev_scale + next.Hsel_lo * next_scale;
	Hsel_hi = prev.Hsel_hi * prev_scale + next.Hsel_hi * next_scale;
	Hsel_over = prev.Hsel_over * prev_scale + next.Hsel_over * next_scale;

	Ssel_lo = prev.Ssel_lo * prev_scale + next.Ssel_lo * next_scale;
	Ssel_hi = prev.Ssel_hi * prev_scale + next.Ssel_hi * next_scale;
	Ssel_over = prev.Ssel_over * prev_scale + next.Ssel_over * next_scale;

	Vsel_lo = prev.Vsel_lo * prev_scale + next.Vsel_lo * next_scale;
	Vsel_hi = prev.Vsel_hi * prev_scale + next.Vsel_hi * next_scale;
	Vsel_over = prev.Vsel_over * prev_scale + next.Vsel_over * next_scale;

	Fsel_lo = prev.Fsel_lo * prev_scale + next.Fsel_lo * next_scale;
	Fsel_mid = prev.Fsel_mid * prev_scale + next.Fsel_mid * next_scale;
	Fsel_hi = prev.Fsel_hi * prev_scale + next.Fsel_hi * next_scale;
	Fsel_over = prev.Fsel_over * prev_scale + next.Fsel_over * next_scale;

	Hadj_val = prev.Hadj_val * prev_scale + next.Hadj_val * next_scale;
	Oadj_val = prev.Oadj_val * prev_scale + next.Oadj_val * next_scale;

	Sadj_gamma = prev.Sadj_gamma * prev_scale + next.Sadj_gamma * next_scale;
	Sadj_lo = prev.Sadj_lo * prev_scale + next.Sadj_lo * next_scale;
	Sadj_hi = prev.Sadj_hi * prev_scale + next.Sadj_hi * next_scale;

	Vadj_gamma = prev.Vadj_gamma * prev_scale + next.Vadj_gamma * next_scale;
	Vadj_lo = prev.Vadj_lo * prev_scale + next.Vadj_lo * next_scale;
	Vadj_hi = prev.Vadj_hi * prev_scale + next.Vadj_hi * next_scale;

	Radj_gamma = prev.Radj_gamma*prev_scale + next.Radj_gamma*next_scale;
	Radj_lo = prev.Radj_lo * prev_scale + next.Radj_lo * next_scale;
	Radj_hi = prev.Radj_hi * prev_scale + next.Radj_hi * next_scale;

	Gadj_gamma = prev.Gadj_gamma * prev_scale + next.Gadj_gamma * next_scale;
	Gadj_lo = prev.Gadj_lo * prev_scale + next.Gadj_lo * next_scale;
	Gadj_hi = prev.Gadj_hi * prev_scale + next.Gadj_hi * next_scale;

	Badj_gamma = prev.Badj_gamma * prev_scale + next.Badj_gamma * next_scale;
	Badj_lo = prev.Badj_lo * prev_scale + next.Badj_lo * next_scale;
	Badj_hi = prev.Badj_hi * prev_scale + next.Badj_hi * next_scale;
}

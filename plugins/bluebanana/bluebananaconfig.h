/*
 * Cinelerra :: Blue Banana - color modification plugin for Cinelerra-CV
 * Copyright (C) 2012-2013 Monty <monty@xiph.org>
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

#ifndef BLUEBANANACONFIG_H
#define BLUEBANANACONFIG_H

#include <stdint.h>
#include "bluebanana.h"
#include "datatype.h"

class BluebananaConfig
{
public:
	BluebananaConfig();
	int equivalent(BluebananaConfig &that);
	void copy_from(BluebananaConfig &that);
	void interpolate(BluebananaConfig &prev,
		BluebananaConfig &next,
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

// non-auto
	int mark;

// auto
	int active;
	int use_mask;
	int capture_mask;
	int invert_selection;

	int Hsel_active;
	float Hsel_lo; /* range of 0 to 360 */
	float Hsel_hi; /* strictly greater than lo, so may extend 0 to 720 */
	float Hsel_over;

	int Ssel_active;
	float Ssel_lo;
	float Ssel_hi;
	float Ssel_over;

	int Vsel_active;
	float Vsel_lo;
	float Vsel_hi;
	float Vsel_over;

	int Fsel_active;
	int Fsel_erode;
	float Fsel_lo;
	float Fsel_mid;
	float Fsel_hi;
	float Fsel_over;

	int Hadj_active;
	float Hadj_val;

	int Sadj_active;
	float Sadj_lo;
	float Sadj_gamma; //*100
	float Sadj_hi;

	int Vadj_active;
	float Vadj_lo;
	float Vadj_gamma;
	float Vadj_hi;

	int Radj_active;
	float Radj_lo;
	float Radj_gamma;
	float Radj_hi;

	int Gadj_active;
	float Gadj_lo;
	float Gadj_gamma;
	float Gadj_hi;

	int Badj_active;
	float Badj_lo;
	float Badj_gamma;
	float Badj_hi;

	int Oadj_active;
	float Oadj_val;
	int ants_counter;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


#endif




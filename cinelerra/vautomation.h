
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

#ifndef VAUTOMATION_H
#define VAUTOMATION_H

#include "automation.h"
#include "edl.inc"
#include "track.inc"

class VAutomation : public Automation
{
public:
	VAutomation(EDL *edl, Track *track);

	void get_projector(double *x,
		double *y,
		double *z,
		ptstime position);

// Get camera coordinates if this is video automation
	void get_camera(double *x,
		double *y,
		double *z,
		ptstime position);
// Size of data in bytes
	size_t get_size();
};

#endif

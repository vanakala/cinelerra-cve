
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

#ifndef FLOATAUTOS_H
#define FLOATAUTOS_H

#include "autos.h"
#include "edl.inc"
#include "guicast.h"
#include "filexml.inc"
#include "floatauto.inc"

class FloatAutos : public Autos
{
public:
	FloatAutos(EDL *edl, 
		Track *track,
// Value for default auto
		float default_);
	~FloatAutos();


	int draw_joining_line(BC_SubWindow *canvas, int vertical, int center_pixel, int x1, int y1, int x2, int y2);
	int get_testy(float slope, int cursor_x, int ax, int ay);
// Return 1 if the automation is constant.
// constant - set to the value if it is constant
	int automation_is_constant(int64_t start, 
		int64_t length, 
		int direction,
		double &constant);
	double get_automation_constant(int64_t start, int64_t end);
// Get value at a specific point.  This needs previous and next stores
// because it is used for every pixel in the drawing function.
	float get_value(int64_t position, 
		int direction,
		FloatAuto* &previous,
		FloatAuto* &next);
	void get_fade_automation(double &slope,
		double &intercept,
		int64_t input_position,
		int64_t &slope_len,
		int direction);
	void get_extents(float *min, 
		float *max,
		int *coords_undefined,
		int64_t unit_start,
		int64_t unit_end);

	void straighten(int64_t start, int64_t end);

	void dump();
	Auto* add_auto(int64_t position, float value);
	Auto* append_auto();
	Auto* new_auto();
	float default_;
};


#endif

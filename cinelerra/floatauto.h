
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

#ifndef FLOATAUTO_H
#define FLOATAUTO_H

// Automation point that takes floating point values

class FloatAuto;

#include "auto.h"
#include "edl.inc"
#include "floatautos.inc"

class FloatAuto : public Auto
{
public:
	FloatAuto() {};
	FloatAuto(EDL *edl, FloatAutos *autos);

	int operator==(Auto &that);
	int operator==(FloatAuto &that);
	int identical(FloatAuto *src);
	void copy_from(Auto *that);
	void copy_from(FloatAuto *that);
	void copy(ptstime start, ptstime end, FileXML *file);
	void load(FileXML *xml);
	void dump(int ident = 0);

	float value_to_percentage();
	float invalue_to_percentage();
	float outvalue_to_percentage();

// "the value" (=payload of this keyframe)
	float get_value() { return this->value; }
	void set_value(float newval);


// Possible policies to handle the tagents for the
// bézier curves connecting adjacent automation points
	enum t_mode
	{
		SMOOTH,     // tangents are coupled in order to yield a smooth curve
		LINEAR,     // tangents always pointing directly to neighbouring automation points
		TFREE,      // tangents on both sides coupled but editable by dragging the handles
		FREE        // tangents on both sides are independent and editable via GUI
	};

	t_mode tangent_mode;
	void change_tangent_mode(t_mode); // recalculates tangents as well
// Control values (y coords of bézier control point), relative to value
	float get_control_in_value() { return this->control_in_value; }
	float get_control_out_value() { return this->control_out_value; }
	void set_control_in_value(float newval);
	void set_control_out_value(float newval);

// get calculated x-position of control points for drawing,
// relative to auto position, in native units of the track.
	ptstime get_control_in_pts() { return this->control_in_pts; }
	ptstime get_control_out_pts() { return this->control_out_pts; }

// X control positions relative to value position for drawing.
	ptstime control_in_pts;
	ptstime control_out_pts;
private:
// Control values are relative to value
	float value;
	float control_in_value;
	float control_out_value;
};

#endif

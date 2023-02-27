// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FLOATAUTO_H
#define FLOATAUTO_H

// Automation point that takes floating point values

#include "auto.h"
#include "edl.inc"
#include "floatauto.inc"
#include "floatautos.inc"

class FloatAuto : public Auto
{
public:
	FloatAuto() {};
	FloatAuto(FloatAutos *autos);
	~FloatAuto();

	void copy_from(Auto *that);
	void copy_from(FloatAuto *that);

// bezier interpolation
	void interpolate_from(Auto *a1, Auto *a2, ptstime pos, Auto *templ = 0);
	void save_xml(FileXML *file);
	void copy(Auto *that, ptstime start, ptstime end);
	void load(FileXML *xml);
	void dump(int ident = 0);

// "the value" (=payload of this keyframe)
	inline double get_raw_value() { return value; }
	double get_value();
	void set_value(double newval, int raw = 0);
	void add_value(double increment);
	inline void set_compat_value(double value) { compat_value = value; }
	size_t get_size();

	tgnt_mode tangent_mode;
	void change_tangent_mode(tgnt_mode); // recalculates tangents as well
	void toggle_tangent_mode();       // cycles through all modes (e.g. by ctrl-click)

// Control values (y coords of bézier control point), relative to value
	double get_control_in_value();
	double get_control_out_value();
	void set_control_in_value(double newval);
	void set_control_out_value(double newval);
	inline double get_raw_control_in_value() { return control_in_value; }
	inline double get_raw_control_out_value() { return control_out_value; }

// get calculated x-position of control points for drawing,
// relative to auto position, in native units of the track.
	ptstime get_control_in_pts() { check_pos(); return this->control_in_pts; }
	ptstime get_control_out_pts() { check_pos(); return this->control_out_pts; }

// define new position and value, re-adjust ctrl point, notify neighbours
	void adjust_to_new_coordinates(ptstime position, double value);

private:
// recalc. ctrl in and out points, if automatic tangent mode (SMOOTH or LINEAR)
	void adjust_tangents();
// recalc. x location of ctrl points, notify neighbours
	void adjust_ctrl_positions();
	void set_ctrl_positions(FloatAuto *prev, FloatAuto *next);
	void calculate_slope(FloatAuto* a1, FloatAuto* a2, double& dvdx, double& dx);
	void check_pos() { if(!PTSEQU(pos_time, pos_valid)) adjust_ctrl_positions(); }
	void tangent_dirty() { pos_valid = -1; }
// check is member of FloatAutos-Collection
	static bool is_floatauto_node(Auto *candidate);
	void handle_automatic_tangent_after_copy();

// Track for loading older versions
	double compat_value;

// X control positions relative to value position for drawing.
	ptstime control_in_pts;
	ptstime control_out_pts;

// Control values are relative to value
	ptstime pos_valid;
	double value;
	double control_in_value;
	double control_out_value;
};

#endif

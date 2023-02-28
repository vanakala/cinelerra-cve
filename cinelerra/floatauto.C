// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "autos.h"
#include "automation.inc"
#include "clip.h"
#include "filexml.h"
#include "floatauto.h"
#include "floatautos.h"
#include "track.h"

FloatAuto::FloatAuto(FloatAutos *autos)
 : Auto((Autos*)autos)
{
	value = 0;
	control_in_value = 0;
	control_out_value = 0;
	control_in_pts = 0;
	control_out_pts = 0;
	compat_value = 0;
	pos_valid = -1;    // "dirty"
	tangent_mode = TGNT_FREE;
//  note: in most cases the tangent_mode-value is set
//        by the method interpolate_from() rsp. copy_from()
}

FloatAuto::~FloatAuto()
{
	// as we are going away, the neighbouring float auto nodes
	// need to re-adjust their ctrl point positions and tangents
	if(is_floatauto_node(this))
	{
		if(next)
			((FloatAuto*)next)->tangent_dirty();
		if(previous)
			((FloatAuto*)previous)->tangent_dirty();
	}
}

inline bool FloatAuto::is_floatauto_node(Auto *candidate)
{
	return (candidate && candidate->autos &&
		AUTOMATION_TYPE_FLOAT == candidate->autos->get_type());
}

void FloatAuto::copy_from(Auto *that)
{
	copy_from((FloatAuto*)that);
}

void FloatAuto::copy_from(FloatAuto *that)
{
	Auto::copy_from(that);
	this->value = that->value;
	this->control_in_value = that->control_in_value;
	this->control_out_value = that->control_out_value;
	this->control_in_pts = that->control_in_pts;
	this->control_out_pts = that->control_out_pts;
	this->tangent_mode = that->tangent_mode;
// note: literate copy, no recalculations
}

// in most cases, we don't want to use the manual tangent modes
// of the left neighbour used as a template for interpolation.
// Rather, we (re)set to automatically smoothed tangents. Note
// auto generated nodes (while tweaking values) indeed are
// inserted by using this "interpolation" approach, thus making
// this defaulting to auto-smooth tangents very important.
inline void FloatAuto::handle_automatic_tangent_after_copy()
{
	if(tangent_mode == TGNT_FREE || tangent_mode == TGNT_TFREE)
	{
		this->tangent_mode = TGNT_SMOOTH;
	}
}

// bézier interpolates this->value and tangents for the given position
// between the positions of a1 and a2. If a1 or a2 are omitted, they default
// to this->previous and this->next. If this FloatAuto has automatic tangents,
// this may trigger re-adjusting of this and its neighbours in this->autos.
// Note while a1 and a2 need not be members of this->autos, automatic
// readjustments are always done to the neighbours in this->autos.
// If the template is given, it will be used to fill out this
// objects fields prior to interpolating.
void FloatAuto::interpolate_from(Auto *a1, Auto *a2, ptstime pos, Auto *templ)
{
	if(!a1) a1 = previous;
	if(!a2) a2 = next;
	Auto::interpolate_from(a1, a2, pos, templ);
	handle_automatic_tangent_after_copy();

	// set this->value using bézier interpolation if possible
	if(is_floatauto_node(a1) && is_floatauto_node(a2) &&
		a1->pos_time <= pos && pos <= a2->pos_time)
	{
		FloatAuto *left = (FloatAuto*)a1;
		FloatAuto *right = (FloatAuto*)a2;

		if(!PTSEQU(pos, pos_time))
		{
			// this may trigger smoothing
			adjust_to_new_coordinates(pos,
				FloatAutos::calculate_bezier(left, right, pos));
		}

		double new_slope = FloatAutos::calculate_bezier_derivation(left, right, pos);

		set_control_in_value(new_slope * control_in_pts);
		set_control_out_value(new_slope * control_out_pts);
	}
	else
		adjust_ctrl_positions(); // implies adjust_tangents()
}

void FloatAuto::change_tangent_mode(tgnt_mode new_mode)
{
	if(new_mode == TGNT_TFREE && !(control_in_pts && control_out_pts))
		new_mode = TGNT_FREE; // only if tangents on both sides...

	tangent_mode = new_mode;
	adjust_tangents();
}

void FloatAuto::toggle_tangent_mode()
{
	switch (tangent_mode)
	{
	case TGNT_SMOOTH:
		change_tangent_mode(TGNT_TFREE);
		break;
	case TGNT_LINEAR:
		change_tangent_mode(TGNT_FREE);
		break;
	case TGNT_TFREE:
		change_tangent_mode(TGNT_LINEAR);
		break;
	case TGNT_FREE:
		change_tangent_mode(TGNT_SMOOTH);
		break;
	}
}

void FloatAuto::set_value(double newvalue, int raw)
{
	if(!raw)
	{
		switch(autos->autoidx)
		{
		case AUTOMATION_CAMERA_X:
		case AUTOMATION_PROJECTOR_X:
			value = newvalue / autos->track->track_w;
			break;

		case AUTOMATION_CAMERA_Y:
		case AUTOMATION_PROJECTOR_Y:
			value = newvalue / autos->track->track_h;
			break;

		default:
			value = newvalue;
			break;
		}
	}
	else
		value = newvalue;

	adjust_tangents();
	if(previous)
		((FloatAuto*)previous)->adjust_tangents();
	if(next)
		((FloatAuto*)next)->adjust_tangents();
}

double FloatAuto::get_value()
{
	switch(autos->autoidx)
	{
	case AUTOMATION_CAMERA_X:
	case AUTOMATION_PROJECTOR_X:
		return value * autos->track->track_w;

	case AUTOMATION_CAMERA_Y:
	case AUTOMATION_PROJECTOR_Y:
		return value * autos->track->track_h;
	}
	return value;
}

void FloatAuto::add_value(double increment)
{
	switch(autos->autoidx)
	{
	case AUTOMATION_CAMERA_X:
	case AUTOMATION_PROJECTOR_X:
		value = increment / autos->track->track_w;
		break;

	case AUTOMATION_CAMERA_Y:
	case AUTOMATION_PROJECTOR_Y:
		value = increment / autos->track->track_h;
		break;
	}
	value += increment;
	adjust_tangents();

	if(previous)
		((FloatAuto*)previous)->adjust_tangents();
	if(next)
		((FloatAuto*)next)->adjust_tangents();
}

void FloatAuto::set_control_in_value(double newvalue)
{
	switch(tangent_mode)
	{
	case TGNT_TFREE:
		control_out_value = control_out_pts * newvalue / control_in_pts;
	default:
		control_in_value = newvalue;
	}
}

void FloatAuto::set_control_out_value(double newvalue)
{
	switch(tangent_mode)
	{
	case TGNT_TFREE:
		control_in_value = control_in_pts * newvalue / control_out_pts;
	default:
		control_out_value = newvalue;
	}
}

double FloatAuto::get_control_in_value()
{
	check_pos();

	switch(autos->autoidx)
	{
	case AUTOMATION_CAMERA_X:
	case AUTOMATION_PROJECTOR_X:
		return control_in_value * autos->track->track_w;

	case AUTOMATION_CAMERA_Y:
	case AUTOMATION_PROJECTOR_Y:
		return control_in_value * autos->track->track_h;
	}
	return control_in_value;
}

double FloatAuto::get_control_out_value()
{
	check_pos();

	switch(autos->autoidx)
	{
	case AUTOMATION_CAMERA_X:
	case AUTOMATION_PROJECTOR_X:
		return control_out_value * autos->track->track_w;

	case AUTOMATION_CAMERA_Y:
	case AUTOMATION_PROJECTOR_Y:
		return control_out_value * autos->track->track_h;
	}
	return control_out_value;
}

inline int sgn(double value)
{
	return (value == 0) ? 0 : (value < 0) ? -1 : 1;
}

inline double weighted_mean(double v1, double v2, double w1, double w2)
{
	if(0.000001 > fabs(w1 + w2))
		return 0;
	else
		return (w1 * v1 + w2 * v2) / (w1 + w2);
}

// recalculates tangents if current mode
// implies automatic adjustment of tangents
void FloatAuto::adjust_tangents()
{
	if(!autos) return;

	if(tangent_mode == TGNT_SMOOTH)
	{
		// normally, one would use the slope of chord between the neighbours.
		// but this could cause the curve to overshot extremal automation nodes.
		// (e.g when setting a fade node at zero, the curve could go negative)
		// we can interpret the slope of chord as a weighted mean value, where
		// the length of the interval is used as weight; we just use other
		// weights: intervall length /and/ reciprocal of slope. So, if the
		// connection to one of the neighbours has very low slope this will
		// dominate the calculated tangent slope at this automation node.
		// if the slope goes beyond the zero line, e.g if left connection
		// has positive and right connection has negative slope, then
		// we force the calculated tangent to be horizontal.

		double s, dxl, dxr, sl, sr;

		calculate_slope((FloatAuto*) previous, this, sl, dxl);
		calculate_slope(this, (FloatAuto*) next, sr, dxr);

		if(0 < sgn(sl) * sgn(sr))
		{
			double wl = fabs(dxl) * (fabs(1.0 / sl) + 1);
			double wr = fabs(dxr) * (fabs(1.0 / sr) + 1);
			s = weighted_mean(sl, sr, wl, wr);
		}
		else
			s = 0; // fixed hoizontal tangent

		control_in_value = s * control_in_pts;
		control_out_value = s * control_out_pts;
	}
	else
	if(tangent_mode == TGNT_LINEAR)
	{
		double g, dx;

		if(previous)
		{
			calculate_slope(this, (FloatAuto*)previous, g, dx);
			control_in_value = g * dx / 3;
		}
		if(next)
		{
			calculate_slope(this, (FloatAuto*)next, g, dx);
			control_out_value = g * dx / 3;
		}
	}
	else
	if(tangent_mode == TGNT_TFREE && control_in_pts && control_out_pts)
	{
		double gl = control_in_value / control_in_pts;
		double gr = control_out_value / control_out_pts;
		double wl = fabs(control_in_value);
		double wr = fabs(control_out_value);
		double g = weighted_mean(gl, gr, wl, wr);

		control_in_value = g * control_in_pts;
		control_out_value = g * control_out_pts;
	}
}

inline void FloatAuto::calculate_slope(FloatAuto *left, FloatAuto *right,
	double &dvdx, double &dx)
{
	dvdx = 0;
	dx = 0;
	if(!left || !right)
		return;

	dx = right->pos_time - left->pos_time;
	double dv = right->value - left->value;
	dvdx = (fabs(dx) < EPSILON) ? 0 : dv / dx;
}

// recalculates location of ctrl points to be
// always 1/3 and 2/3 of the distance to the
// next neighbours. The reason is: for this special
// distance the bézier function yields x(t) = t, i.e.
// we can use the y(t) as if it was a simple function y(x).

// This adjustment is done only on demand and involves
// updating neighbours and adjust_tangents() as well.
void FloatAuto::adjust_ctrl_positions()
{
	FloatAuto *prev = (FloatAuto*)this->previous;
	FloatAuto *next = (FloatAuto*)this->next;

	if(prev)
	{
		set_ctrl_positions(prev, this);
		prev->adjust_tangents();
	}
	else // disable tangent on left side
		control_in_pts = 0;

	if(next)
	{
		set_ctrl_positions(this, next);
		next->adjust_tangents();
	}
	else // disable right tangent
		control_out_pts = 0;

	this->adjust_tangents();
	pos_valid = pos_time;
// tangents up-to-date
}

inline void redefine_tangent(ptstime &old_pos, ptstime new_pos, double &ctrl_val)
{
	if(old_pos > EPSILON)
		ctrl_val *= new_pos / old_pos;
	old_pos = new_pos;
}

inline void FloatAuto::set_ctrl_positions(FloatAuto *prev, FloatAuto* next)
{
	ptstime distance = next->pos_time - prev->pos_time;

	redefine_tangent(prev->control_out_pts, +distance / 3, prev->control_out_value);
	redefine_tangent(next->control_in_pts, -distance / 3, next->control_in_value);
}

// define new position and value in one step, do necessary re-adjustments
void FloatAuto::adjust_to_new_coordinates(ptstime position, double value)
{
	this->value = value;
	this->pos_time = position;
	adjust_ctrl_positions();
}

void FloatAuto::save_xml(FileXML *file)
{
	file->tag.set_title("AUTO");
	file->tag.set_property("POSTIME", pos_time);
	file->tag.set_property("VALUE", value);
	file->tag.set_property("CONTROL_IN_VALUE", control_in_value / 2.0);  // compatibility, see below
	file->tag.set_property("CONTROL_OUT_VALUE", control_out_value / 2.0);
	file->tag.set_property("TANGENT_MODE", tangent_mode);
	file->append_tag();
	file->tag.set_title("/AUTO");
	file->append_tag();
	file->append_newline();
}

void FloatAuto::copy(Auto *src, ptstime start, ptstime end)
{
	FloatAuto *that = (FloatAuto*)src;

	pos_time = that->pos_time - start;
	value = that->value;
	control_in_value = that->control_in_value;
	control_out_value = that->control_out_value;
	tangent_mode = that->tangent_mode;
}

void FloatAuto::load(FileXML *file)
{
	value = file->tag.get_property("VALUE", value);
	control_in_value = file->tag.get_property("CONTROL_IN_VALUE", control_in_value);
	control_out_value = file->tag.get_property("CONTROL_OUT_VALUE", control_out_value);
	tangent_mode = (tgnt_mode)file->tag.get_property("TANGENT_MODE", TGNT_FREE);

	// Compatibility to old session data format:
	// Versions previous to the bezier auto patch (Jun 2006) applied a factor 2
	// to the y-coordinates of ctrl points while calculating the bezier function.
	// To retain compatibility, we now apply this factor while loading
	control_in_value *= 2.0;
	control_out_value *= 2.0;

	if(compat_value && (value < -1.0 || value > 1.0))
	{
		value /= compat_value;
		control_in_value /= compat_value;
		control_out_value /= compat_value;
		compat_value = 0;
	}
// restore ctrl positions and adjust tangents if necessary
	adjust_ctrl_positions();
}

size_t FloatAuto::get_size()
{
	return sizeof(*this);
}

void FloatAuto::dump(int ident)
{
	const char *s;

	switch(tangent_mode)
	{
	case TGNT_SMOOTH:
		s = "smooth";
		break;
	case TGNT_LINEAR:
		s = "linear";
		break;
	case TGNT_TFREE:
		s = "tangent";
		break;
	case TGNT_FREE:
		s = "disjoint";
		break;
	default:
		s = "tangent unknown";
		break;
	}
	printf("%*sFloatAuto %p: pos_time %.3f value %.3f\n",
		ident, "", this, pos_time, value);
	ident += 2;
	printf("%*scontrols in %.2f/%.3f, out %.2f/%.3f %s\n", ident, "",
		control_in_value, control_in_pts, control_out_value, control_out_pts, s);
}

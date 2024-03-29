// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef MASKAUTO_H
#define MASKAUTO_H

#include "arraylist.h"
#include "auto.h"
#include "maskauto.inc"
#include "maskautos.inc"
#include "track.inc"

class MaskPoint
{
public:
	MaskPoint();

	int operator==(MaskPoint& ptr);
	size_t get_size();
	void dump(int indent = 0);

	double submask_x;
	double submask_y;
// Incoming acceleration
	double control_x1, control_y1;
// Outgoing acceleration
	double control_x2, control_y2;
};

class SubMask
{
public:
	SubMask(MaskAuto *keyframe);

	int operator==(SubMask& ptr);
	void copy_from(SubMask& ptr);
	void load(FileXML *file, Track *track);
	void save_xml(FileXML *file);
	MaskPoint *append_point(MaskPoint *templ);

	size_t get_size();
	void dump(int indent = 0);

	ArrayList<MaskPoint*> points;
	MaskAuto *keyframe;
};

class MaskAuto : public Auto
{
public:
	MaskAuto(MaskAutos *autos);
	~MaskAuto();

	int operator==(Auto &that);
	int identical(MaskAuto *src);
	void load(FileXML *file);
	void save_xml(FileXML *file);
	void copy(Auto *that, ptstime start, ptstime end);
	void copy_from(Auto *src);
	void interpolate_from(Auto *a1, Auto *a2, ptstime position, Auto *templ = 0);
	void interpolate_values(ptstime pts, int *new_value, int *new_feather);
	void copy_from(MaskAuto *src);

// Retrieve submask with clamping
	SubMask *get_submask(int number);
// Create submask if it does not exist
	SubMask *create_submask(int number);

	size_t get_size();
	void dump(int indent = 0);

	ArrayList<SubMask*> masks;

	int feather;
// 0 - 100
	int value;
	int apply_before_plugins;
};

#endif

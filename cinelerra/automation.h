// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef AUTOMATION_H
#define AUTOMATION_H

#include "arraylist.h"
#include "autoconf.inc"
#include "automation.inc"
#include "autos.inc"
#include "datatype.h"
#include "edl.inc"
#include "filexml.inc"
#include "mwindow.inc"
#include "track.inc"

#include <stdint.h>

// Match the clipping at per cinelerra/virtualanode.C which contains:
//  if(fade_value <= INFINITYGAIN) fade_value = 0;
//  in reality, this should be matched to a user-defined minimum in the preferences
#define AUTOMATIONCLAMPS(value, autogrouptype) \
	if(autogrouptype == AUTOGROUPTYPE_AUDIO_FADE && value <= INFINITYGAIN) \
		value = INFINITYGAIN; \
	if (autogrouptype == AUTOGROUPTYPE_VIDEO_FADE) \
		CLAMP(value, 0, 100); \
	if (autogrouptype == AUTOGROUPTYPE_ZOOM && value < 0) \
		value = 0;

#define AUTOMATIONVIEWCLAMPS(value, autogrouptype) \
	if (autogrouptype == AUTOGROUPTYPE_ZOOM && value < 0)\
		value = 0;

struct automation_def
{
	const char *name;
	const char *xml_title;
	const char *xml_visible;
	int is_visble;
	int autogrouptype;
	int type;
	int color;
	int preoperation;
	int operation;
};

struct autogrouptype_def
{
	const char *titlemax;
	const char *titlemin;
	int fixedrange;
};

class Automation
{
public:
	Automation(EDL *edl, Track *track);
	virtual ~Automation();

	int autogrouptype(int autoidx, Track *track);
	void equivalent_output(Automation *automation, ptstime *result);
	virtual Automation& operator=(Automation& automation);
	virtual void copy_from(Automation *automation);
	int load(FileXML *file, int operation = PASTE_ALL);
// For copy automation, copy, and save
	void save_xml(FileXML *xml);
	void copy(Automation *automation, ptstime start, ptstime end);

// Get projector coordinates if this is video automation
	virtual void get_projector(double *x,
		double *y,
		double *z,
		ptstime position) {};
// Get camera coordinates if this is video automation
	virtual void get_camera(double *x,
		double *y,
		double *z,
		ptstime position) {};

// Returns the point to restart background rendering at.
// -1 means nothing changed.
	void clear(ptstime start,
		ptstime end,
		AutoConf *autoconf, 
		int shift_autos);
	void clear_after(ptstime pts);
	void straighten(ptstime start, ptstime end);
	void paste_silence(ptstime start, ptstime end);
	void insert_track(Automation *automation, 
		ptstime start,
		ptstime length,
		int overwrite = 0);
	virtual void get_extents(double *min,
		double *max,
		int *coords_undefined,
		ptstime start,
		ptstime end,
		int autogrouptype);
	ptstime get_length();
	virtual size_t get_size();
	static const char *name(int index);
	static int index(const char *name);

	void dump(int indent = 0);

	Autos *autos[AUTOMATION_TOTAL];
	static struct automation_def automation_tbl[];
	static struct autogrouptype_def autogrouptypes[];
	EDL *edl;
	Track *track;
};

#endif

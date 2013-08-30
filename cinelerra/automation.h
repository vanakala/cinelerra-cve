
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

#ifndef AUTOMATION_H
#define AUTOMATION_H

#include "arraylist.h"
#include "autoconf.inc"
#include "automation.inc"
#include "autos.inc"
#include "datatype.h"
#include "edl.inc"
#include "filexml.inc"
#include "maxchannels.h"
#include "module.inc"
#include "track.inc"

#include <stdint.h>

// Match the clipping at per cinelerra/virtualanode.C which contains:
//  if(fade_value <= INFINITYGAIN) fade_value = 0;
//  in reality, this should be matched to a user-defined minimum in the preferences
#define AUTOMATIONCLAMPS(value, autogrouptype)				\
	if (autogrouptype == AUTOGROUPTYPE_AUDIO_FADE && value <= INFINITYGAIN) \
		value = INFINITYGAIN;					\
	if (autogrouptype == AUTOGROUPTYPE_VIDEO_FADE)			\
		CLAMP(value, 0, 100);					\
	if (autogrouptype == AUTOGROUPTYPE_ZOOM && value < 0)		\
		value = 0;

#define AUTOMATIONVIEWCLAMPS(value, autogrouptype)			\
	if (autogrouptype == AUTOGROUPTYPE_ZOOM && value < 0)		\
		value = 0;

class Automation
{
public:
	static int autogrouptypes_fixedrange[];
	Automation(EDL *edl, Track *track);
	virtual ~Automation();

	int autogrouptype(int autoidx, Track *track);
	void equivalent_output(Automation *automation, ptstime *result);
	virtual Automation& operator=(Automation& automation);
	virtual void copy_from(Automation *automation);
	int load(FileXML *file);
// For copy automation, copy, and save
	int copy(ptstime start,
		ptstime end,
		FileXML *xml);

// For paste automation only
	int paste(ptstime start,
		ptstime length,
		double scale,
		FileXML *file,
		AutoConf *autoconf);
	void paste(ptstime start,
		ptstime length,
		FileXML *file);

// Get projector coordinates if this is video automation
	virtual void get_projector(float *x, 
		float *y, 
		float *z, 
		ptstime position) {};
// Get camera coordinates if this is video automation
	virtual void get_camera(float *x, 
		float *y, 
		float *z, 
		ptstime position) {};

// Returns the point to restart background rendering at.
// -1 means nothing changed.
	void clear(ptstime start,
		ptstime end,
		AutoConf *autoconf, 
		int shift_autos);
	void straighten(ptstime start, 
		ptstime end,
		AutoConf *autoconf);
	void paste_silence(ptstime start, ptstime end);
	void insert_track(Automation *automation, 
		ptstime start,
		ptstime length);
	virtual void get_extents(float *min, 
		float *max,
		int *coords_undefined,
		ptstime start,
		ptstime end,
		int autogrouptype);

	ptstime pos2pts(posnum position);
	posnum pts2pos(ptstime position);
	void dump(int indent);

	Autos *autos[AUTOMATION_TOTAL];

	EDL *edl;
	Track *track;
};

#endif

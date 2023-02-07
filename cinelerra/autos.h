// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef AUTOS_H
#define AUTOS_H

// Base class for automation lists.

#include "auto.h"
#include "edl.inc"
#include "datatype.h"
#include "bcsignals.h"
#include "filexml.inc"
#include "track.inc"

class Autos : public List<Auto>
{
public:
	Autos(EDL *edl, Track *track);
	~Autos();

	void equivalent_output(Autos *autos, ptstime startproject, ptstime *result);
	void copy_from(Autos *autos);
	virtual void copy_values(Autos *autos) {};
	virtual Auto* new_auto() { return 0; };
// Get existing auto on or before position.
// If use_default is true, return default_auto if none exists 
// on or before position.
// Return 0 if none exists and use_default is false.
// If &current is nonzero it is used as a starting point for searching.
	Auto* get_prev_auto(ptstime position, Auto* current = 0);
	Auto* get_prev_auto(Auto* current);
	Auto* get_next_auto(ptstime position, Auto* current = 0);
// Determine if a keyframe exists before creating it.
	int auto_exists_for_editing(ptstime position);
// Returns auto at exact position, null if non-existent. ignores autokeyframming and align on frames
	Auto* get_auto_at_position(ptstime position = -1);

// Get keyframe for editing with automatic creation if enabled
	Auto* get_auto_for_editing(ptstime position = -1);

// Insert keyframe at the point if it doesn't exist
// Interpolate its value if possible
	Auto* insert_auto(ptstime position, Auto *templ = 0);
	void insert_track(Autos *automation, ptstime start, ptstime length,
		int overwrite = 0);
	virtual void load(FileXML *xml);
	void paste(ptstime start, FileXML *file);

// Round pts to track units, add delta units to position
	ptstime unit_round(ptstime pts, int delta = 0);
// Calculate min & max position for drag
	virtual void drag_limits(Auto *current, ptstime *prev, ptstime *next);

// Returns a type enumeration
	int get_type();
	virtual void get_extents(double *min, double *max,
		int *coords_undefined, ptstime start, ptstime end) {};

	void clear_all();
	void insert(ptstime start, ptstime end);
	void paste_silence(ptstime start, ptstime end);

	void save_xml(FileXML *xml);
	virtual void copy(Autos *autos, ptstime start, ptstime end);

// Stores the background rendering position in result
	void clear(ptstime start, ptstime end, int shift_autos);
	void clear_after(ptstime pts);
	virtual void straighten(ptstime start, ptstime end) {};

	int save(FileXML *xml);
	int release_auto();
	void shift_all(ptstime difference);

	Auto* append_auto();

// rendering utilities
	void get_neighbors(ptstime start, ptstime end,
			Auto **before, Auto **after);
// 1 if automation doesn't change
	virtual int automation_is_constant(ptstime start, ptstime end) { return 0; };

	Auto* autoof(ptstime position);   // return nearest auto equal to or after position
                                          // 0 if after all autos
	Auto* nearest_before(ptstime position);    // return nearest auto before or 0
	Auto* nearest_after(ptstime position);     // return nearest auto after or 0

	virtual void dump(int indent = 0) {};

	EDL *edl;
	Track *track;

	int autoidx;
	int autogrouptype;
	int type;

	ptstime base_pts;
};

#endif

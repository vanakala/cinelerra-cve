
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

#ifndef AUTOS_H
#define AUTOS_H

// Base class for automation lists.

#include "auto.h"
#include "edl.inc"
#include "datatype.h"
#include "guicast.h"
#include "filexml.inc"
#include "track.inc"

class Autos : public List<Auto>
{
public:
	Autos(EDL *edl, 
		Track *track);

	virtual ~Autos();
	virtual void create_objects();
	void equivalent_output(Autos *autos, ptstime startproject, ptstime *result);
	void copy_from(Autos *autos);
	virtual Auto* new_auto();
// Get existing auto on or before position.
// If use_default is true, return default_auto if none exists 
// on or before position.
// Return 0 if none exists and use_default is false.
// If &current is nonzero it is used as a starting point for searching.
	Auto* get_prev_auto(ptstime position, Auto* &current, int use_default = 1);
	Auto* get_prev_auto(Auto* &current);
	Auto* get_next_auto(ptstime position, Auto* &current, int use_default = 1);
// Determine if a keyframe exists before creating it.
	int auto_exists_for_editing(ptstime position);
// Returns auto at exact position, null if non-existent. ignores autokeyframming and align on frames
	Auto* get_auto_at_position(ptstime position = -1);

// Get keyframe for editing with automatic creation if enabled
	Auto* get_auto_for_editing(ptstime position = -1);

// Insert keyframe at the point if it doesn't exist
	Auto* insert_auto(ptstime position);
// Insert keyframe at the point if it doesn't exist
// Interpolate it insead of copying
	Auto* insert_auto_for_editing(ptstime position);
	void insert_track(Autos *automation, 
		ptstime start,
		ptstime length,
		int replace_default);
	virtual int load(FileXML *xml);
	void paste(ptstime start,
		ptstime length,
		double scale, 
		FileXML *file, 
		int default_only);
	void remove_nonsequential(Auto *keyframe);
	void optimize();

// Returns a type enumeration
	int get_type();
	ptstime get_length();
	virtual void get_extents(float *min, 
		float *max,
		int *coords_undefined,
		ptstime start,
		ptstime end) {};

	EDL *edl;
	Track *track;
// Default settings if no autos.
// Having a persistent keyframe created problems when files were loaded and
// we wanted to keep only 1 auto.
// Default auto has position 0 except in effects, where multiple default autos
// exist.
	Auto *default_auto;

	int autoidx;
	int autogrouptype;
	int type;

	virtual void dump() {};

	int clear_all();
	int insert(ptstime start, ptstime end);
	int paste_silence(ptstime start, ptstime end);
	int copy(ptstime start,
		ptstime end, 
		FileXML *xml, 
		int default_only,
		int autos_only);
// Stores the background rendering position in result
	void clear(ptstime start,
		ptstime end,
		int shift_autos);
	virtual void straighten(ptstime start, ptstime end) {};

	int save(FileXML *xml);
	int release_auto();
	ptstime pos2pts(posnum position);
	posnum pts2pos(ptstime position);
	Auto* append_auto();

// rendering utilities
	int get_neighbors(ptstime start, ptstime end,
			Auto **before, Auto **after);
// 1 if automation doesn't change
	virtual int automation_is_constant(ptstime start, ptstime end) { return 0; };
	virtual double get_automation_constant(ptstime start, ptstime end) { return 0; };

	Auto* autoof(ptstime position);   // return nearest auto equal to or after position
                                          // 0 if after all autos
	Auto* nearest_before(ptstime position);    // return nearest auto before or 0
	Auto* nearest_after(ptstime position);     // return nearest auto after or 0

};

#endif

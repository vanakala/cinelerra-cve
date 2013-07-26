
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

#ifndef EDIT_H
#define EDIT_H

#include "asset.inc"
#include "datatype.h"
#include "edl.inc"
#include "guicast.h"
#include "edits.inc"
#include "filexml.inc"
#include "keyframe.inc"
#include "mwindow.inc"
#include "plugin.inc"
#include "track.inc"
#include "transition.inc"

// Generic edit of something

class Edit : public ListItem<Edit>
{
public:
	Edit(EDL *edl, Edits *edits);
	Edit(EDL *edl, Track *track);
	virtual ~Edit();

	virtual ptstime length(void);
	ptstime end_pts(void);
	virtual void copy_from(Edit *edit);
	virtual int identical(Edit &edit);
	virtual Edit& operator=(Edit& edit);
// Called by Edits and PluginSet
	virtual void equivalent_output(Edit *edit, ptstime *result);
	virtual int operator==(Edit& edit);
// When inherited by a plugin need to resample keyframes
	virtual void synchronize_params(Edit *edit);
// Used by Edits::insert_edits to shift plugin keyframes
	virtual void shift_keyframes(ptstime postime) {};

// Get size of frame to draw on timeline
	double picon_w(void);
	int picon_h(void);
	int copy(ptstime start, ptstime end, FileXML *xml, const char *output_path);

// Shift in time
	virtual void shift(ptstime difference);
	void insert_transition(const char *title, KeyFrame *default_keyframe);
	void detach_transition(void);
// Determine if silence depending on existance of asset or plugin title
	virtual int silence(void);

// Media edit information
// Units are seconds
// Start of edit in source file in seconds
	ptstime source_pts;
// Start of edit in project
	ptstime project_pts;

// Channel or layer of source
	int channel;
// ID for resource pixmaps
	int id;
// User defined title for timeline
	char user_title[BCTEXTLEN];


// Transition if one is present at the beginning of this edit
// This stores the length of the transition
	Transition *transition;
	EDL *edl;

	Edits *edits;

	Track *track;

// Asset is 0 if silence, otherwise points an object in edl->assets
	Asset *asset;

// ============================= initialization

	ptstime load_properties(FileXML *xml, ptstime project_pts);
	virtual void load_properties_derived(FileXML *xml) {};

// ============================= editing

	virtual int copy_properties_derived(FileXML *xml, ptstime len_in_selection) { return 0; };
	virtual ptstime get_source_end(ptstime default_value);
	void dump(int indent = 0);
};

#endif

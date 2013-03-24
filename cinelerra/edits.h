
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

#ifndef EDITS_H
#define EDITS_H


#include "asset.inc"
#include "assets.inc"
#include "edl.inc"
#include "guicast.h"
#include "edit.h"
#include "filexml.inc"
#include "linklist.h"
#include "track.inc"
#include "transition.inc"


// Generic list of edits of something

class Edits : public List<Edit>
{
public:
	Edits(EDL *edl, Track *track, Edit *default_edit);
	virtual ~Edits();

	void equivalent_output(Edits *edits, ptstime *result);
	virtual void copy_from(Edits *edits);
	virtual Edits& operator=(Edits& edits);
// Editing
	void insert_edits(Edits *edits, ptstime position);
	void insert_asset(Asset *asset, ptstime length_time, ptstime postime, int track_number);
// Split edit containing position.
// Return the second edit in the split.
	Edit* split_edit(ptstime postime);
// Create a blank edit in the native data format
	void clear_handle(ptstime start,
		ptstime end,
		int actions,
		ptstime &distance);
	virtual Edit* create_edit() { return 0; };
// Insert a 0 length edit at the position
	Edit* insert_new_edit(ptstime postime);
	int save(FileXML *xml, const char *output_path);
	int copy(ptstime start, ptstime end, FileXML *xml, const char *output_path);
// Clear region of edits
	virtual void clear(ptstime start, ptstime end);
// Clear edits and plugins for a handle modification
	virtual void clear_recursive(ptstime start,
		ptstime end,
		int actions,
		Edits *trim_edits);
	virtual void shift_keyframes_recursive(ptstime position, ptstime length);
	virtual void shift_effects_recursive(ptstime position, ptstime length);
// Does not return an edit - does what it says, nothing more or less
	void paste_silence(ptstime start, ptstime end);
// Returns the newly created edit
	Edit *create_and_insert_edit(ptstime start, ptstime end);

// Shift edits on or after position by distance
// Return the edit now on the position.
	virtual Edit* shift(ptstime position, ptstime difference);

	EDL *edl;
	Track *track;


// ============================= initialization commands ====================
	Edits() { printf("default edits constructor called\n"); };

// ================================== file operations

	void load(FileXML *xml, int track_offset);
	void load_edit(FileXML *xml, ptstime &project_time, int track_offset);

	virtual Edit* append_new_edit() { return 0; };
	virtual Edit* insert_edit_after(Edit *previous_edit) { return 0; };
	virtual int load_edit_properties(FileXML *xml) {};


// ==================================== accounting

	Edit* editof(ptstime postime, int use_nudge);
// Return an edit if position is over an edit and the edit has a source file
	Edit* get_playable_edit(ptstime postime, int use_nudge);
	ptstime length();         // end position of last edit
	virtual void dump(int indent = 0);

// ==================================== editing

	int modify_handles(ptstime oldposition,
		ptstime newposition,
		int currentend,
		int edit_mode,
		int actions,
		Edits *trim_edits);
	virtual void optimize(void);

	ptstime loaded_length;
};



#endif


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



#define LAST_VIRTUAL_LENGTH 1000000000

// Generic list of edits of something

class Edits : public List<Edit>
{
public:
	Edits(EDL *edl, Track *track, Edit *default_edit);
	virtual ~Edits();	

	void equivalent_output(Edits *edits, int64_t *result);
	virtual void copy_from(Edits *edits);
	virtual Edits& operator=(Edits& edits);
// Editing
	void insert_edits(Edits *edits, int64_t position);
	void insert_asset(Asset *asset, int64_t length, int64_t sample, int track_number);
// Split edit containing position.
// Return the second edit in the split.
	Edit* split_edit(int64_t position);
// Create a blank edit in the native data format
	int clear_handle(double start, 
		double end, 
		int edit_plugins, 
		double &distance);
	virtual Edit* create_edit() { return 0; };
// Insert a 0 length edit at the position
	Edit* insert_new_edit(int64_t sample);
	int save(FileXML *xml, char *output_path);
	int copy(int64_t start, int64_t end, FileXML *xml, char *output_path);
// Clear region of edits
	virtual void clear(int64_t start, int64_t end);
// Clear edits and plugins for a handle modification
	virtual void clear_recursive(int64_t start, 
		int64_t end, 
		int edit_edits,
		int edit_labels, 
		int edit_plugins,
		Edits *trim_edits);
	virtual void shift_keyframes_recursive(int64_t position, int64_t length);
	virtual void shift_effects_recursive(int64_t position, int64_t length);
// Does not return an edit - does what it says, nothing more or less
	void paste_silence(int64_t start, int64_t end);
// Returns the newly created edit
	Edit *create_and_insert_edit(int64_t start, int64_t end);

	void resample(double old_rate, double new_rate);
// Shift edits on or after position by distance
// Return the edit now on the position.
	virtual Edit* shift(int64_t position, int64_t difference);

	EDL *edl;
	Track *track;










// ============================= initialization commands ====================
	Edits() { printf("default edits constructor called\n"); };

// ================================== file operations

	int load(FileXML *xml, int track_offset);
	int load_edit(FileXML *xml, int64_t &startproject, int track_offset);

	virtual Edit* append_new_edit() {};
	virtual Edit* insert_edit_after(Edit *previous_edit) { return 0; };
	virtual int load_edit_properties(FileXML *xml) {};


// ==================================== accounting

	Edit* editof(int64_t position, int direction, int use_nudge);
// Return an edit if position is over an edit and the edit has a source file
	Edit* get_playable_edit(int64_t position, int use_nudge);
//	int64_t total_length();
	int64_t length();         // end position of last edit

// ==================================== editing

	int modify_handles(double oldposition, 
		double newposition, 
		int currentend,
		int edit_mode, 
		int edit_edits,
		int edit_labels,
		int edit_plugins,
		Edits *trim_edits);
	virtual int optimize();

	int64_t loaded_length;
private:
	virtual int clone_derived(Edit* new_edit, Edit* old_edit) {};
};



#endif

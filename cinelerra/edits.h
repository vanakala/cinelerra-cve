// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef EDITS_H
#define EDITS_H

#include "asset.inc"
#include "edl.inc"
#include "edit.h"
#include "filexml.inc"
#include "linklist.h"
#include "track.inc"

// Generic list of edits of something
class Edits : public List<Edit>
{
public:
	Edits(EDL *edl, Track *track);

	void equivalent_output(Edits *edits, ptstime *result);
	void copy_from(Edits *edits);
	Edits& operator=(Edits& edits);
// Editing
	void insert_edits(Edits *edits, ptstime position,
		ptstime duration = -1, int insert = 0);
	void insert_asset(Asset *asset, ptstime length_time,
		ptstime postime, int track_number, int overwrite = 0);
// Split edit containing position.
// Return the second edit in the split.
// if force is set create new edit always 
	Edit* split_edit(ptstime postime, int force = 0);
// Create a blank edit in the native data format
	void clear_handle(ptstime start,
		ptstime end,
		int actions,
		ptstime &distance);
	Edit* create_edit();
	void save_xml(FileXML *xml, const char *output_path);
	void copy(Edits *edits, ptstime start, ptstime end);
// Clear region of edits
	void clear(ptstime start, ptstime end);
// Clear edit after
	void clear_after(ptstime pts);
// Shift edits starting from edit
	void shift_edits(Edit *edit, ptstime diff);
// Does not return an edit - does what it says, nothing more or less
	void paste_silence(ptstime start, ptstime end);

// Sanitize edits
	void cleanup();

	EDL *edl;
	Track *track;

// ==================================== accounting
	Edit* editof(ptstime postime);
// Return an edit if position is over an edit and the edit has a source file
	Edit* get_playable_edit(ptstime postime);
	ptstime length();         // end position of last edit
	void dump(int indent = 0);

// ==================================== editing
	void modify_handles(ptstime oldposition,
		ptstime newposition,
		int edit_handle,
		int edit_mode);
	void move_edits(Edit *current_edit, ptstime newposition);
	ptstime adjust_position(ptstime oldposition, ptstime newposition,
		int edit_handle, int edit_mode);
	ptstime limit_move(Edit *edit, ptstime newposition, int check_end = 0);
	ptstime limit_source_move(Edit *edit, ptstime newposition);

// ================================== file operations
	void load(FileXML *xml);
// Returns the size of data in bytes
	size_t get_size();

private:
	ptstime load_edit(FileXML *xml, ptstime project_time);
};

#endif

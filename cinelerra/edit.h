
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
#include "edl.inc"
#include "guicast.h"
#include "edits.inc"
#include "filexml.inc"
#include "mwindow.inc"
#include "plugin.inc"
#include "track.inc"
#include "transition.inc"

// UNITS ARE SAMPLES FOR AUDIO / FRAMES FOR VIDEO
// zoom_units was mwindow->zoom_sample for AEdit

// Generic edit of something

class Edit : public ListItem<Edit>
{
public:
	Edit(EDL *edl, Edits *edits);
	Edit(EDL *edl, Track *track);
	Edit();
	virtual ~Edit();

	void reset();
	virtual void copy_from(Edit *edit);
	virtual int identical(Edit &edit);
	virtual Edit& operator=(Edit& edit);
// Called by Edits and PluginSet
	virtual void equivalent_output(Edit *edit, int64_t *result);
	virtual int operator==(Edit& edit);
// When inherited by a plugin need to resample keyframes
	virtual void synchronize_params(Edit *edit);
// Used by Edits::insert_edits to shift plugin keyframes
	virtual void shift_keyframes(int64_t position) {};

// Get size of frame to draw on timeline
	double picon_w();
	int picon_h();
	double frame_w();
	double frames_per_picon();
	int copy(int64_t start, int64_t end, FileXML *xml, char *output_path);
// When inherited by a plugin need to resample keyframes
	virtual void resample(double old_rate, double new_rate) {};

// Shift in time
	virtual void shift(int64_t difference);
	int shift_start_in(int edit_mode, 
		int64_t newposition, 
		int64_t oldposition,
		int edit_edits,
		int edit_labels,
		int edit_plugins,
		Edits *trim_edits);
	int shift_start_out(int edit_mode, 
		int64_t newposition, 
		int64_t oldposition,
		int edit_edits,
		int edit_labels,
		int edit_plugins,
		Edits *trim_edits);
	int shift_end_in(int edit_mode, 
		int64_t newposition, 
		int64_t oldposition,
		int edit_edits,
		int edit_labels,
		int edit_plugins,
		Edits *trim_edits);
	int shift_end_out(int edit_mode, 
		int64_t newposition, 
		int64_t oldposition,
		int edit_edits,
		int edit_labels,
		int edit_plugins,
		Edits *trim_edits);

	void insert_transition(char  *title);
	void detach_transition();
// Determine if silence depending on existance of asset or plugin title
	virtual int silence();

// Media edit information
// Units are native units for the track.
// Start of edit in source file normalized to project sample rate.
// Normalized because all the editing operations clip startsource relative
// to the project sample rate;
	int64_t startsource;  
// Start of edit in project file.
	int64_t startproject;    
// # of units in edit.
	int64_t length;  
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

	int load_properties(FileXML *xml, int64_t &startproject);
	virtual int load_properties_derived(FileXML *xml) {};

// ============================= drawing

	virtual int draw(int flash, int center_pixel, int x, int w, int y, int h, int set_index_file) {};
	virtual int set_index_file(int flash, int center_pixel, int x, int y, int w, int h) {};
	int draw_transition(int flash, int center_pixel, int x, int w, int y, int h, int set_index_file);

	int draw_handles(BC_SubWindow *canvas, float view_start, float view_units, float zoom_units, int view_pixels, int center_pixel);
	int draw_titles(BC_SubWindow *canvas, float view_start, float zoom_units, int view_pixels, int center_pixel);

// ============================= editing

	virtual int copy_properties_derived(FileXML *xml, int64_t length_in_selection) {};

	int popup_transition(float view_start, float zoom_units, int cursor_x, int cursor_y);

// Return 1 if the left handle was selected 2 if the right handle was selected
	int select_handle(float view_start, float zoom_units, int cursor_x, int cursor_y, int64_t &selection);
	virtual int get_handle_parameters(int64_t &left, int64_t &right, int64_t &left_sample, int64_t &right_sample, float view_start, float zoom_units) {};
	virtual int64_t get_source_end(int64_t default_);
	int dump();
	virtual int dump_derived() {};

};





#endif

#ifndef EDIT_H
#define EDIT_H

#include "assets.inc"
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
	virtual void equivalent_output(Edit *edit, long *result);
	virtual int operator==(Edit& edit);
// When inherited by a plugin need to resample keyframes
	virtual void synchronize_params(Edit *edit);
// Used by Edits::insert_edits to shift plugin keyframes
	virtual void shift_keyframes(long position) {};

// Get size of frame to draw on timeline
	double picon_w();
	int picon_h();
	double frame_w();
	double frames_per_picon();
	int copy(long start, long end, FileXML *xml, char *output_path);
// When inherited by a plugin need to resample keyframes
	virtual void resample(double old_rate, double new_rate) {};

// Shift in time
	virtual void shift(long difference);
	int shift_start_in(int edit_mode, 
		long newposition, 
		long oldposition,
		int edit_edits,
		int edit_labels,
		int edit_plugins);
	int shift_start_out(int edit_mode, 
		long newposition, 
		long oldposition,
		int edit_edits,
		int edit_labels,
		int edit_plugins);
	int shift_end_in(int edit_mode, 
		long newposition, 
		long oldposition,
		int edit_edits,
		int edit_labels,
		int edit_plugins);
	int shift_end_out(int edit_mode, 
		long newposition, 
		long oldposition,
		int edit_edits,
		int edit_labels,
		int edit_plugins);

	void insert_transition(char  *title);
	void detach_transition();
// Determine if silence depending on existance of asset or plugin title
	virtual int silence();

// Media edit information
// Units are native units for the track.
// Start of edit in source file normalized to project sample rate.
// Normalized because all the editing operations clip startsource relative
// to the project sample rate;
	long startsource;  
// Start of edit in project file.
	long startproject;    
// # of units in edit.
	long length;  
// Channel or layer of source
	int channel;
// ID for resource pixmaps
	int id;

// Transition if one is present at the beginning of this edit
// This stores the length of the transition
	Transition *transition;
	EDL *edl;

	Edits *edits;

	Track *track;

// Asset is 0 if silence, otherwise points an object in edl->assets
	Asset *asset;














// ============================= initialization

	int load_properties(FileXML *xml, long &startproject);
	virtual int load_properties_derived(FileXML *xml) {};

// ============================= drawing

	virtual int draw(int flash, int center_pixel, int x, int w, int y, int h, int set_index_file) {};
	virtual int set_index_file(int flash, int center_pixel, int x, int y, int w, int h) {};
	int draw_transition(int flash, int center_pixel, int x, int w, int y, int h, int set_index_file);

	int draw_handles(BC_SubWindow *canvas, float view_start, float view_units, float zoom_units, int view_pixels, int center_pixel);
	int draw_titles(BC_SubWindow *canvas, float view_start, float zoom_units, int view_pixels, int center_pixel);

// ============================= editing

	virtual int copy_properties_derived(FileXML *xml, long length_in_selection) {};

	int popup_transition(float view_start, float zoom_units, int cursor_x, int cursor_y);

// Return 1 if the left handle was selected 2 if the right handle was selected
	int select_handle(float view_start, float zoom_units, int cursor_x, int cursor_y, long &selection);
	virtual int get_handle_parameters(long &left, long &right, long &left_sample, long &right_sample, float view_start, float zoom_units) {};
	virtual long get_source_end(long default_);
	int dump();
	virtual int dump_derived() {};

};





#endif

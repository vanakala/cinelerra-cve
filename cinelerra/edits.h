#ifndef EDITS_H
#define EDITS_H


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
	Edits(EDL *edl, Track *track);
	virtual ~Edits();	

	void equivalent_output(Edits *edits, long *result);
	virtual Edits& operator=(Edits& edits);
// Editing
	void insert_edits(Edits *edits, long position);
	void insert_asset(Asset *asset, long length, long sample, int track_number);
// Split edit containing position.
// Return the second edit in the split.
	Edit* split_edit(long position);
// Create a blank edit in the native data format
	int clear_handle(double start, 
		double end, 
		int edit_plugins, 
		double &distance);
	virtual Edit* create_edit() { return 0; };
// Insert a 0 length edit at the position
	Edit* insert_new_edit(long sample);
	int save(FileXML *xml, char *output_path);
	int copy(long start, long end, FileXML *xml, char *output_path);
// Clear region of edits
	virtual void clear(long start, long end);
// Clear edits and plugins for a handle modification
	virtual void clear_recursive(long start, 
		long end, 
		int edit_edits,
		int edit_labels, 
		int edit_plugins);
	virtual void shift_keyframes_recursive(long position, long length);
	virtual void shift_effects_recursive(long position, long length);
// Returns the newly created edit
	Edit* paste_silence(long start, long end);
	void resample(double old_rate, double new_rate);
// Shift edits on or after position by distance
// Return the edit now on the position.
	virtual Edit* shift(long position, long difference);

	EDL *edl;
	Track *track;










// ============================= initialization commands ====================
	Edits() { printf("default edits constructor called\n"); };

// ================================== file operations

	int load(FileXML *xml, int track_offset);
	int load_edit(FileXML *xml, long &startproject, int track_offset);

	virtual Edit* append_new_edit() {};
	virtual Edit* insert_edit_after(Edit *previous_edit) { return 0; };
	virtual int load_edit_properties(FileXML *xml) {};


// ==================================== accounting

	Edit* editof(long position, int direction);
// Return an edit if position is over an edit and the edit has a source file
	Edit* get_playable_edit(long position);
//	long total_length();
	long length();         // end position of last edit

// ==================================== editing

// inserts space at the desired location and returns the edit before the space
// fills end of track if range is after track
	int modify_handles(double oldposition, 
		double newposition, 
		int currentend,
		int edit_mode, 
		int edit_edits,
		int edit_labels,
		int edit_plugins);
	virtual int optimize();


private:
	virtual int clone_derived(Edit* new_edit, Edit* old_edit) {};
};



#endif

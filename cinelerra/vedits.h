#ifndef VEDITS_H
#define VEDITS_H

#include "edits.h"
#include "edl.inc"
#include "filexml.h"
#include "mwindow.inc"
#include "vtrack.inc"



class VEdits : public Edits
{
public:
	VEdits(EDL *edl, Track *track);


// Editing
	Edit* create_edit();












	VEdits() {printf("default edits constructor called\n");};
	~VEdits() {};


// ========================================= editing

	Edit* append_new_edit();
	Edit* insert_edit_after(Edit* previous_edit);

	VTrack *vtrack;
};


#endif

#ifndef AEDITS_H
#define AEDITS_H

#include "atrack.inc"
#include "edits.h"
#include "edl.inc"
#include "filexml.inc"
#include "mwindow.inc"

class AEdits : public Edits
{
public:
	AEdits(EDL *edl, Track *track);


// Editing
	Edit* create_edit();











	AEdits() {printf("default edits constructor called\n");};
	~AEdits() {};	


// ======================================= editing

	Edit* append_new_edit();
	Edit* insert_edit_after(Edit* previous_edit);
	int clone_derived(Edit* new_edit, Edit* old_edit);

private:
	ATrack *atrack;
};



#endif

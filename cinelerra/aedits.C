#include "aedit.h"
#include "aedits.h"
#include "edits.h"
#include "filexml.h"
#include "mwindow.h"
#include "mainsession.h"


AEdits::AEdits(EDL *edl, Track *track)
 : Edits(edl, track)
{
}



Edit* AEdits::create_edit()
{
	return new AEdit(edl, this);
}


Edit* AEdits::insert_edit_after(Edit* previous_edit)
{
	AEdit *current = new AEdit(edl, this);
	
	insert_after(previous_edit, current);
//printf("AEdits::insert_edit_after %p %p\n", current->track, current->edits);

	return (Edit*)current;
}


















Edit* AEdits::append_new_edit()
{
	AEdit *current;
	append(current = new AEdit(edl, this));
	return (Edit*)current;
}

int AEdits::clone_derived(Edit* new_edit, Edit* old_edit)
{
	return 0;
}

#include "assets.h"
#include "edit.h"
#include "vedit.h"
#include "vedits.h"
#include "filesystem.h"
#include "mwindow.h"
#include "loadfile.h"
#include "vtrack.h"








VEdits::VEdits(EDL *edl, Track *track)
 : Edits(edl, track)
{
}

Edit* VEdits::create_edit()
{
	return new VEdit(edl, this);
}

Edit* VEdits::insert_edit_after(Edit* previous_edit)
{
	VEdit *current = new VEdit(edl, this);

	List<Edit>::insert_after(previous_edit, current);

//printf("VEdits::insert_edit_after %p %p\n", current->track, current->edits);
	return (Edit*)current;
}









Edit* VEdits::append_new_edit()
{
	VEdit *current;
	List<Edit>::append(current = new VEdit(edl, this));
	return (Edit*)current;
}



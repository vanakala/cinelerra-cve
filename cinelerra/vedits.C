
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

#include "assets.h"
#include "edit.h"
#include "vedit.h"
#include "vedits.h"
#include "filesystem.h"
#include "mwindow.h"
#include "loadfile.h"
#include "vtrack.h"








VEdits::VEdits(EDL *edl, Track *track)
 : Edits(edl, track, create_edit())
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



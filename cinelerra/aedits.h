
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

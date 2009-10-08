
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

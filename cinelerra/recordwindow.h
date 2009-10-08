
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

#ifndef RECORDWINDOW_H
#define RECORDWINDOW_H


#include "guicast.h"
#include "mwindow.inc"
#include "record.inc"



// Dialog for record file format.


#define RECORD_WINDOW_WIDTH 410
#define RECORD_WINDOW_HEIGHT 360
		

class RecordWindow : public BC_Window
{
public:
	RecordWindow(MWindow *mwindow, Record *record, int w, int h);
	~RecordWindow();

	int create_objects();

	MWindow *mwindow;
	FormatTools *format_tools;
	Record *record;
	LoadMode *loadmode;
};



class RecordToTracks : public BC_CheckBox
{
public:
	RecordToTracks(Record *record, int default_);
	~RecordToTracks();

	int handle_event();
	Record *record;
};


#endif

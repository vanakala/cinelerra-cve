
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

#include "formattools.h"
#include "language.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "record.h"
#include "recordwindow.h"
#include "videodevice.inc"






RecordWindow::RecordWindow(MWindow *mwindow, Record *record, int x, int y)
 : BC_Window(PROGRAM_NAME ": Record", 
	x,
	y,
 	RECORD_WINDOW_WIDTH, 
	RECORD_WINDOW_HEIGHT,
	(int)BC_INFINITY,
	(int)BC_INFINITY,
	0,
	0,
	1)
{
	this->mwindow = mwindow;
	this->record = record;
}

RecordWindow::~RecordWindow()
{
	delete format_tools;
//	delete loadmode;
}



int RecordWindow::create_objects()
{
//printf("RecordWindow::create_objects 1\n");
	add_subwindow(new BC_Title(5, 5, _("Select a file to record to:")));

//printf("RecordWindow::create_objects 1\n");
	int x = 5, y = 25;
	format_tools = new FormatTools(mwindow,
					this, 
					record->default_asset);
//printf("RecordWindow::create_objects 1\n");
	format_tools->create_objects(x, 
					y, 
					1, 
					1, 
					1, 
					1, 
					1, 
					1,
					/* record->fixed_compression */ 0,
					1,
					0,
					0);
//printf("RecordWindow::create_objects 1\n");

// Not the same as creating a new file at each label.
// Load mode is now located in the RecordGUI
	x = 10;
//	loadmode = new LoadMode(this, x, y, &record->load_mode, 1);
// 	loadmode->create_objects();

	add_subwindow(new BC_OKButton(this));
//printf("RecordWindow::create_objects 1\n");
	add_subwindow(new BC_CancelButton(this));
//printf("RecordWindow::create_objects 1\n");
	show_window();
//printf("RecordWindow::create_objects 2\n");
	return 0;
}







RecordToTracks::RecordToTracks(Record *record, int default_)
 : BC_CheckBox(200, 270, default_) { this->record = record; }
RecordToTracks::~RecordToTracks() 
{}
int RecordToTracks::handle_event()
{
	record->to_tracks = get_value();
}


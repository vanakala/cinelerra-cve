
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

#ifndef VWINDOW_H
#define VWINDOW_H

#include "asset.inc"
#include "clipedit.inc"
#include "edl.inc"
#include "mwindow.inc"
#include "thread.h"
#include "transportque.inc"
#include "vplayback.inc"
#include "vtracking.inc"
#include "vwindowgui.inc"

class VWindow : public Thread
{
public:
	VWindow(MWindow *mwindow);
	~VWindow();

	void load_defaults();
	int create_objects();
	void run();
// Change source to asset, creating a new EDL
	void change_source(Asset *asset);
// Change source to EDL
	void change_source(EDL *edl);
// Change source to master EDL's vwindow EDL after a load.
	void change_source();
// Change source to folder and item number
	void change_source(char *folder, int item);
// Remove source
	void remove_source();
// Returns private EDL of VWindow
// If an asset is dropped in, a new VWindow EDL is created in the master EDL
// and this points to it.
// If a clip is dropped in, it points to the clip EDL.
	EDL* get_edl();
// Returns last argument of change_source or 0 if it was an EDL
	Asset* get_asset();
	void update(int do_timebar);
		
	void update_position(int change_type = CHANGE_NONE,
		int use_slider = 1,
		int update_slider = 0);
	void set_inpoint();
	void set_outpoint();
	void clear_inpoint();
	void clear_outpoint();
	void copy();
	void splice_selection();
	void overwrite_selection();	
	void delete_edl();
	void goto_start();
	void goto_end();


	VTracking *playback_cursor;

// Number of source in GUI list
	MWindow *mwindow;
	VWindow *vwindow;
	VWindowGUI *gui;
	VPlayback *playback_engine;
	ClipEdit *clip_edit;
// Object being played back.

// Pointer to asset for accounting
	Asset *asset;
};


#endif

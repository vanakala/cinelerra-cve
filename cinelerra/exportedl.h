
/*
 * CINELERRA
 * Copyright (C) 2006 Andraz Tori
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

#ifndef EXPORTEDL_H
#define EXPORTEDL_H


#include "asset.inc"
#include "bitspopup.h"
#include "browsebutton.h"
#include "cache.inc"
#include "compresspopup.h"
#include "condition.inc"
#include "edit.inc"
#include "errorbox.inc"
#include "file.inc"
#include "guicast.h"
#include "mutex.inc"
#include "mwindow.inc"

#define EDLTYPE_CMX3600 1

class ExportEDLPathText;
class ExportEDLWindowTrackList;
class ExportEDLWindow;

class ExportEDLAsset 
{
public:
	ExportEDLAsset(MWindow *mwindow, EDL *edl);
	~ExportEDLAsset();
	// EDL being exported
	EDL *edl;
	// path to file
	char path[BCTEXTLEN];
	// type of EDL
	int edl_type;
	
	// We are currently exporting a track at once
	int track_number;
	
	
	int export_it();
	MWindow *mwindow;
	
	int load_defaults();
	int save_defaults();
private:
	int edit_to_timecodes(Edit *edit, char *sourceinpoint, char *sourceoutpoint, char *destinpoint, char *destoutpoint, char *reel_name);
	void double_to_CMX3600(double seconds, double frame_rate, char *str);
	
};

class ExportEDLItem : public BC_MenuItem
{
public:
	ExportEDLItem(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};



class ExportEDL : public Thread
{
public:
	ExportEDL(MWindow *mwindow);
	~ExportEDL();

	void start_interactive();
	void run();


// Force filename to have a 0 padded number if rendering to a list.

	MWindow *mwindow;
//	Mutex *package_lock, *counter_lock;
// Copy of mwindow preferences
//	Preferences *preferences;
// Total selection to render in seconds
	double total_start, total_end;

// Current open RenderWindow
	ExportEDLWindow *exportedl_window;
	ExportEDLAsset *exportasset;

};


class ExportEDLWindow : public BC_Window
{
public:
	ExportEDLWindow(MWindow *mwindow, ExportEDL *exportedl, ExportEDLAsset *exportasset);
	~ExportEDLWindow();

	int create_objects();

	ExportEDLAsset *exportasset;

	BrowseButton *path_button;
	ExportEDLPathText *path_textbox;
	BC_RecentList *path_recent;
	ExportEDLWindowTrackList *track_list;

	ArrayList<BC_ListBoxItem*> items_tracks[2];

	MWindow *mwindow;
};





class ExportEDLPathText : public BC_TextBox
{
public:
	ExportEDLPathText(int x, int y, ExportEDLWindow *window);
	~ExportEDLPathText();
	int handle_event();
	
	ExportEDLWindow *window;
};

class ExportEDLWindowTrackList : public BC_ListBox
{
public:
	ExportEDLWindowTrackList(ExportEDLWindow *window, 
		int x, 
		int y, 
		int w, 
		int h, 
		ArrayList<BC_ListBoxItem*> *track_list);

	int handle_event();
	ExportEDLWindow *window;
};









#endif

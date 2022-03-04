// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2006 Andraz Tori

#ifndef EXPORTEDL_H
#define EXPORTEDL_H

#include "browsebutton.inc"
#include "bcrecentlist.inc"
#include "bcwindow.h"
#include "bcmenuitem.h"
#include "cache.inc"
#include "edit.inc"
#include "file.inc"
#include "thread.h"

#define EDLTYPE_CMX3600 1

class ExportEDLPathText;
class ExportEDLWindowTrackList;
class ExportEDLWindow;

class ExportEDLAsset 
{
public:
	ExportEDLAsset(EDL *edl);

	// EDL being exported
	EDL *edl;
	// path to file
	char path[BCTEXTLEN];
	// type of EDL
	int edl_type;

	// We are currently exporting a track at once
	int track_number;
	void export_it();

	void load_defaults();
	void save_defaults();
private:
	void edit_to_timecodes(Edit *edit, char *sourceinpoint, char *sourceoutpoint, char *destinpoint, char *destoutpoint, char *reel_name);
	void double_to_CMX3600(ptstime seconds, double frame_rate, char *str);
};

class ExportEDLItem : public BC_MenuItem
{
public:
	ExportEDLItem();

	int handle_event();
};


class ExportEDL : public Thread
{
public:
	ExportEDL();

	void start_interactive();
	void run();

// Force filename to have a 0 padded number if rendering to a list.

// Current open RenderWindow
	ExportEDLWindow *exportedl_window;
	ExportEDLAsset *exportasset;
};


class ExportEDLWindow : public BC_Window
{
public:
	ExportEDLWindow(int x, int y, ExportEDL *exportedl,
		ExportEDLAsset *exportasset);

	ExportEDLAsset *exportasset;

	BrowseButton *path_button;
	ExportEDLPathText *path_textbox;
	BC_RecentList *path_recent;
	ExportEDLWindowTrackList *track_list;

	ArrayList<BC_ListBoxItem*> items_tracks[2];
};


class ExportEDLPathText : public BC_TextBox
{
public:
	ExportEDLPathText(int x, int y, ExportEDLWindow *window);

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
};

#endif

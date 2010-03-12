
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

#ifndef FORMATPRESETS_H
#define FORMATPRESETS_H


#include "edl.inc"
#include "formatpresets.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "new.inc"
#include "setformat.inc"


class FormatPresets
{
public:
	FormatPresets(MWindow *mwindow,
		NewWindow *new_gui, 
		SetFormatWindow *format_gui, 
		int x, 
		int y);
	virtual ~FormatPresets();

	void create_objects();
	FormatPresetItem* find_preset(EDL *edl);
	const char* get_preset_text(EDL *edl);

// New preset selected
	virtual int handle_event();
	virtual EDL* get_edl();
	
	MWindow *mwindow;
	BC_WindowBase *gui_base;
	NewWindow *new_gui;
	SetFormatWindow *format_gui;
	FormatPresetsText *text;
	FormatPresetsPulldown *pulldown;
	int x, y;
	ArrayList<FormatPresetItem*> preset_items;
};



class FormatPresetsText : public BC_TextBox
{
public:
	FormatPresetsText(MWindow *mwindow, 
		FormatPresets *gui,
		int x, 
		int y);
	int handle_event();
// Find the listbox item which corresponds to the values in the edl.
	FormatPresets *gui;
	MWindow *mwindow;
};

class FormatPresetsPulldown : public BC_ListBox
{
public:
	FormatPresetsPulldown(MWindow *mwindow, 
		FormatPresets *gui, 
		int x, 
		int y);
	int handle_event();
	MWindow *mwindow;
	FormatPresets *gui;
};

class FormatPresetItem : public BC_ListBoxItem
{
public:
	FormatPresetItem(MWindow *mwindow, FormatPresets *gui, char *text);
	~FormatPresetItem();

	MWindow *mwindow;
	FormatPresets *gui;
// Storage of the values for the preset
	EDL *edl;
};





#endif

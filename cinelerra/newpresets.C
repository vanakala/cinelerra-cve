
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

#include "newpresets.h"




NewPresetsText::NewPresetsText(MWindow *mwindow, 
	NewWindow *new_gui, 
	SetFormatWindow *format_gui,
	int x, 
	int y)
 : BC_TextBox(x, y, 200, 1, window->get_preset_text())
{
	this->mwindow = mwindow;
	this->new_gui = new_gui;
	this->format_gui = format_gui;
}

int NewPresetsText::handle_event()
{
	return 1;
}





NewPresetsPulldown::NewPresetsPulldown(MWindow *mwindow, NewWindow *window, int x, int y)
 : BC_ListBox(x, 
		y, 
		200, 
		200,
		LISTBOX_TEXT,                   // Display text list or icons
		(ArrayList<BC_ListBoxItem*>*)&window->preset_items, // Each column has an ArrayList of BC_ListBoxItems.
		0,             // Titles for columns.  Set to 0 for no titles
		0,                // width of each column
		1,                      // Total columns.
		0,                    // Pixel of top of window.
		1)
{
	this->mwindow = mwindow;
	this->window = window;
}
int NewPresetsPulldown::handle_event()
{
	NewPresetItem *preset = ((NewPresetItem*)get_selection(0, 0));
	window->new_edl->copy_all(preset->edl);
	window->update();
	window->presets_text->update(preset->get_text());
	return 1;
}

NewPresetItem::NewPresetItem(MWindow *mwindow, NewWindow *window, char *text)
 : BC_ListBoxItem(text)
{
	this->mwindow = mwindow;
	this->window = window;
	edl = new EDL;
	edl->create_objects();
	edl->copy_all(window->new_edl);
}

NewPresetItem::~NewPresetItem()
{
	delete edl;
}



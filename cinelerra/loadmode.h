
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

#ifndef LOADMODE_H
#define LOADMODE_H

#include "guicast.h"
#include "loadmode.inc"
#include "mwindow.inc"

class LoadModeListBox;

class LoadModeItem : public BC_ListBoxItem
{
public:
	LoadModeItem(char *text, int value);
	int value;
};

class LoadMode
{
public:
	LoadMode(MWindow *mwindow,
		BC_WindowBase *window, 
		int x, 
		int y, 
		int *output,
		int use_nothing);
	~LoadMode();
	
	int create_objects();
	int reposition_window(int x, int y);
	static int calculate_h(BC_WindowBase *gui);
	int get_h();
	int get_x();
	int get_y();

	char* mode_to_text();

	BC_Title *title;
	BC_TextBox *textbox;
	LoadModeListBox *listbox;
	MWindow *mwindow;
	BC_WindowBase *window;
	int x;
	int y;
	int *output;
	int use_nothing;
	ArrayList<LoadModeItem*> load_modes;
};

class LoadModeListBox : public BC_ListBox
{
public:
	LoadModeListBox(BC_WindowBase *window, LoadMode *loadmode, int x, int y);
	~LoadModeListBox();

	int handle_event();

	BC_WindowBase *window;
	LoadMode *loadmode;
};

#endif

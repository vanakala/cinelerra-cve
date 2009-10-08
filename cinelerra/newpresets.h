
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

#ifndef NEWPRESETS_H
#define NEWPRESETS_H


#include "guicast.h"
#include "new.inc"
#include "setformat.inc"

class NewPresetsText : public BC_TextBox
{
public:
	NewPresetsText(MWindow *mwindow, NewWindow *window, int x, int y);
	int handle_event();
	MWindow *mwindow;
	NewWindow *window;
};

class NewPresetsPulldown : public BC_ListBox
{
public:
	NewPresetsPulldown(MWindow *mwindow, NewWindow *window, int x, int y);
	int handle_event();
	MWindow *mwindow;
	NewWindow *window;
};

class NewPresetItem : public BC_ListBoxItem
{
public:
	NewPresetItem(MWindow *mwindow, NewWindow *window, char *text);
	~NewPresetItem();

	MWindow *mwindow;
	NewWindow *window;
	EDL *edl;
};





#endif

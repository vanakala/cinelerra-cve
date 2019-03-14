
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

#include "loadmode.inc"
#include "mwindow.inc"
#include "selection.h"

class LoadMode
{
public:
	LoadMode(BC_WindowBase *window,
		int x, 
		int y, 
		int *output,
		int use_nothing);

	friend class InsertionModeSelection;

	void reposition_window(int x, int y);
	static const char* name(int mode);
private:
	static const struct selection_int insertion_modes[];
	BC_Title *title;
	InsertionModeSelection *modeselection;
};

class InsertionModeSelection : public Selection
{
public:
	InsertionModeSelection(int x, int y,
		BC_WindowBase *base, int *value, int optmask);

	void update(int value);
};

#endif

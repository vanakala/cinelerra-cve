
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

#ifndef LABELNAVIGATE_H
#define LABELNAVIGATE_H

class PrevLabel;
class NextLabel;

#include "guicast.h"
#include "mbuttons.inc"
#include "mwindow.inc"

class LabelNavigate
{
public:
	LabelNavigate(MWindow *mwindow, MButtons *gui, int x, int y);
	~LabelNavigate();

	void create_objects();
	
	PrevLabel *prev_label;
	NextLabel *next_label;
	MWindow *mwindow;
	MButtons *gui;
	int x;
	int y;
};

class PrevLabel : public BC_Button
{
public:
	PrevLabel(MWindow *mwindow, LabelNavigate *navigate, int x, int y);
	~PrevLabel();

	int handle_event();

	MWindow *mwindow;
	LabelNavigate *navigate;
};

class NextLabel : public BC_Button
{
public:
	NextLabel(MWindow *mwindow, LabelNavigate *navigate, int x, int y);
	~NextLabel();

	int handle_event();

	MWindow *mwindow;
	LabelNavigate *navigate;
};

#endif

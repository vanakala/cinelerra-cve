
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

#ifndef TRACKSCROLL_H
#define TRACKSCROLL_H


#include "guicast.h"
#include "mwindow.inc"
#include "mwindowgui.inc"

class TrackScroll : public BC_ScrollBar
{
public:
	TrackScroll(MWindow *mwindow, MWindowGUI *gui, int x, int y, int h);
	~TrackScroll();

	int create_objects(int top, int bottom);
	int resize_event();
	int flip_vertical(int top, int bottom);
	int update();               // reflect new track view
	long get_distance();
	int handle_event();

	MWindowGUI *gui;
	MWindow *mwindow;
	long old_position;
};

#endif

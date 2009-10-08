
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

#ifndef MAINXSCROLL_H
#define MAINXSCROLL_H

#include "guicast.h"
#include "mwindow.inc"
#include "mwindowgui.inc"

class SampleScroll : public BC_ScrollBar
{
public:
	SampleScroll(MWindow *mwindow, MWindowGUI *gui, int x, int y, int w);
	~SampleScroll();

	int create_objects();
	int flip_vertical();
	int in_use();
	int resize_event();
	int set_position();
	int handle_event();
	double oldposition;

private:
	MWindowGUI *gui;
	MWindow *mwindow;
};



#endif

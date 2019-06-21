
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

#ifndef MAINCURSOR_H
#define MAINCURSOR_H

#include "datatype.h"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "mutex.h"

class MainCursor
{
public:
	MainCursor(MWindow *mwindow, MWindowGUI *gui);

	void repeat_event(int duration);
	void draw(int flash);
	void hide(int do_plugintoggles = 1);
	void flash();
	void activate();
	void deactivate();
	void show(int do_plugintoggles = 1);
	void restore(int do_plugintoggles);
	void update();
	void focus_out_event();

	MWindow *mwindow;
	MWindowGUI *gui;
	int visible;
	ptstime selectionstart, selectionend;
	int pixel2, pixel1;
	int active;
	int playing_back;
private:
	Mutex cursor_lock;
};

#endif


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

#ifndef LEVELWINDOWGUI_H
#define LEVELWINDOWGUI_H

class LevelWindowReset;

#include "guicast.h"
#include "levelwindow.inc"
#include "maxchannels.h"
#include "meterpanel.inc"
#include "mwindow.inc"

class LevelWindowGUI : public BC_Window
{
public:
	LevelWindowGUI(MWindow *mwindow, LevelWindow *thread);
	~LevelWindowGUI();

	int create_objects();
	int resize_event(int w, int h);
	int translation_event();
	int close_event();
	int reset_over();
	int keypress_event();

	MWindow *mwindow;

	MeterPanel *panel;
	LevelWindow *thread;
};


#endif

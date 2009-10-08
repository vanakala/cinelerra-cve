
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

#ifndef BLURZOOMWINDOW_H
#define BLURZOOMWINDOW_H

#include "guicast.h"

class BlurZoomThread;
class BlurZoomWindow;

#include "filexml.h"
#include "mutex.h"
#include "blurzoom.h"

class BlurZoomThread : public Thread
{
public:
	BlurZoomThread(BlurZoomMain *client);
	~BlurZoomThread();

	void run();

// prevent loading data until the GUI is started
 	Mutex gui_started, completion;
	BlurZoomMain *client;
	BlurZoomWindow *window;
};

class BlurZoomWindow : public BC_Window
{
public:
	BlurZoomWindow(BlurZoomMain *client, int x, int y);
	~BlurZoomWindow();

	int create_objects();
	int close_event();

	BlurZoomMain *client;
};








#endif


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

#ifndef FLIPWINDOW_H
#define FLIPWINDOW_H

#include "bcbase.h"

class InvertThread;
class InvertWindow;

#include "filexml.h"
#include "mutex.h"
#include "invert.h"

class InvertThread : public Thread
{
public:
	InvertThread(InvertMain *client);
	~InvertThread();

	void run();

	Mutex gui_started; // prevent loading data until the GUI is started
	InvertMain *client;
	InvertWindow *window;
};

class InvertToggle;

class InvertWindow : public BC_Window
{
public:
	InvertWindow(InvertMain *client);
	~InvertWindow();
	
	int create_objects();
	int close_event();
	
	InvertMain *client;
	InvertToggle *invert;
};

class InvertToggle : public BC_CheckBox
{
public:
	InvertToggle(InvertMain *client, int *output, int x, int y);
	~InvertToggle();
	int handle_event();

	InvertMain *client;
	int *output;
};


#endif


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

#ifndef BLURWINDOW_H
#define BLURWINDOW_H

#include "bcbase.h"

class OilThread;
class OilWindow;

#include "filexml.h"
#include "mutex.h"
#include "oil.h"

class OilThread : public Thread
{
public:
	OilThread(OilMain *client);
	~OilThread();

	void run();

	Mutex gui_started; // prevent loading data until the GUI is started
	OilMain *client;
	OilWindow *window;
};

class OilRadius;
class OilIntensity;

class OilWindow : public BC_Window
{
public:
	OilWindow(OilMain *client);
	~OilWindow();
	
	int create_objects();
	int close_event();
	
	OilMain *client;
	OilRadius *radius;
	OilIntensity *use_intensity;
};

class OilRadius : public BC_IPot
{
public:
	OilRadius(OilMain *client, int x, int y);
	~OilRadius();
	int handle_event();

	OilMain *client;
};

class OilIntensity : public BC_CheckBox
{
public:
	OilIntensity(OilMain *client, int x, int y);
	~OilIntensity();
	int handle_event();

	OilMain *client;
};


#endif

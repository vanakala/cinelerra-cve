
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

#ifndef YUVWINDOW_H
#define YUVWINDOW_H

#include "bcbase.h"

class YUVThread;
class YUVWindow;

#include "filexml.h"
#include "mutex.h"
#include "yuv.h"

class YUVThread : public Thread
{
public:
	YUVThread(YUVMain *client);
	~YUVThread();

	void run();

	Mutex gui_started; // prevent loading data until the GUI is started
	YUVMain *client;
	YUVWindow *window;
};

class YSlider;
class USlider;
class VSlider;
class AutomatedFn;

class YUVWindow : public BC_Window
{
public:
	YUVWindow(YUVMain *client);
	~YUVWindow();

	int create_objects();
	int close_event();

	YUVMain *client;
	YSlider *y_slider;
	USlider *u_slider;
	VSlider *v_slider;
	AutomatedFn *automation[3];
};

class YSlider : public BC_ISlider
{
public:
	YSlider(YUVMain *client, int x, int y);
	~YSlider();
	int handle_event();

	YUVMain *client;
};

class USlider : public BC_ISlider
{
public:
	USlider(YUVMain *client, int x, int y);
	~USlider();
	int handle_event();

	YUVMain *client;
};

class VSlider : public BC_ISlider
{
public:
	VSlider(YUVMain *client, int x, int y);
	~VSlider();
	int handle_event();

	YUVMain *client;
};

class AutomatedFn : public BC_CheckBox
{
public:
	AutomatedFn(YUVMain *client, YUVWindow *window, int x, int y, int number);
	~AutomatedFn();
	int handle_event();

	YUVMain *client;
	YUVWindow *window;
	int number;
};


#endif

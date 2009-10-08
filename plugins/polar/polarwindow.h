
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

#ifndef POLARWINDOW_H
#define POLARWINDOW_H

#include "bcbase.h"

class PolarThread;
class PolarWindow;

#include "filexml.h"
#include "mutex.h"
#include "polar.h"

PLUGIN_THREAD_HEADER(PolarMain, PolarThread, PolarWindow)

class DepthSlider;
class AngleSlider;
class AutomatedFn;

class PolarWindow : public BC_Window
{
public:
	PolarWindow(PolarMain *client);
	~PolarWindow();
	
	int create_objects();
	int close_event();
	
	PolarMain *client;
	DepthSlider *depth_slider;
	AngleSlider *angle_slider;
	AutomatedFn *automation[2];
};

class DepthSlider : public BC_ISlider
{
public:
	DepthSlider(PolarMain *client, int x, int y);
	~DepthSlider();
	int handle_event();

	PolarMain *client;
};

class AngleSlider : public BC_ISlider
{
public:
	AngleSlider(PolarMain *client, int x, int y);
	~AngleSlider();
	int handle_event();

	PolarMain *client;
};

class AutomatedFn : public BC_CheckBox
{
public:
	AutomatedFn(PolarMain *client, PolarWindow *window, int x, int y, int number);
	~AutomatedFn();
	int handle_event();

	PolarMain *client;
	PolarWindow *window;
	int number;
};


#endif

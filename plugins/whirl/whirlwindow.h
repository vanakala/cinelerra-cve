
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

#ifndef WHIRLWINDOW_H
#define WHIRLWINDOW_H

#include "guicast.h"

class WhirlThread;
class WhirlWindow;

#include "filexml.h"
#include "mutex.h"
#include "whirl.h"

class WhirlThread : public Thread
{
public:
	WhirlThread(WhirlMain *client);
	~WhirlThread();

	void run();

	Mutex gui_started; // prevent loading data until the GUI is started
	WhirlMain *client;
	WhirlWindow *window;
};

class AngleSlider;
class PinchSlider;
class RadiusSlider;
class AutomatedFn;

class WhirlWindow : public BC_Window
{
public:
	WhirlWindow(WhirlMain *client);
	~WhirlWindow();
	
	int create_objects();
	int close_event();
	
	WhirlMain *client;
	AngleSlider *angle_slider;
	PinchSlider *pinch_slider;
	RadiusSlider *radius_slider;
	AutomatedFn *automation[3];
};

class AngleSlider : public BC_ISlider
{
public:
	AngleSlider(WhirlMain *client, int x, int y);
	~AngleSlider();
	int handle_event();

	WhirlMain *client;
};

class PinchSlider : public BC_ISlider
{
public:
	PinchSlider(WhirlMain *client, int x, int y);
	~PinchSlider();
	int handle_event();

	WhirlMain *client;
};

class RadiusSlider : public BC_ISlider
{
public:
	RadiusSlider(WhirlMain *client, int x, int y);
	~RadiusSlider();
	int handle_event();

	WhirlMain *client;
};

class AutomatedFn : public BC_CheckBox
{
public:
	AutomatedFn(WhirlMain *client, WhirlWindow *window, int x, int y, int number);
	~AutomatedFn();
	int handle_event();

	WhirlMain *client;
	WhirlWindow *window;
	int number;
};


#endif

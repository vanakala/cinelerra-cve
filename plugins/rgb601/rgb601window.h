
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

#ifndef RGB601WINDOW_H
#define RGB601WINDOW_H

#include "guicast.h"

class RGB601Thread;
class RGB601Window;

#include "filexml.h"
#include "mutex.h"
#include "rgb601.h"

PLUGIN_THREAD_HEADER(RGB601Main, RGB601Thread, RGB601Window)

class RGB601Direction : public BC_CheckBox
{
public:
	RGB601Direction(RGB601Window *window, int x, int y, int *output, int true_value, char *text);
	~RGB601Direction();
	
	int handle_event();
	RGB601Window *window;
	int *output;
	int true_value;
};

class RGB601Window : public BC_Window
{
public:
	RGB601Window(RGB601Main *client, int x, int y);
	~RGB601Window();
	
	int create_objects();
	int close_event();
	void update();

	RGB601Main *client;
	BC_CheckBox *forward, *reverse;
};

#endif

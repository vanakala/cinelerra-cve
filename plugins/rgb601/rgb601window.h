
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
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER

class RGB601Direction : public BC_CheckBox
{
public:
	RGB601Direction(RGB601Window *window, int x, int y, int *output, int true_value, const char *text);
	~RGB601Direction();

	int handle_event();
	RGB601Window *window;
	int *output;
	int true_value;
};

class RGB601Window : public PluginWindow
{
public:
	RGB601Window(RGB601Main *plugin, int x, int y);
	~RGB601Window();

	void update();

	RGB601Main *client;
	BC_CheckBox *forward, *reverse;
	PLUGIN_GUI_CLASS_MEMBERS
};

#endif

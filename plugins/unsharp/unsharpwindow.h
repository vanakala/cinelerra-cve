
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

#ifndef UNSHARPWINDOW_H
#define UNSHARPWINDOW_H

#include "guicast.h"
#include "unsharp.inc"
#include "unsharpwindow.inc"

class UnsharpRadius : public BC_FPot
{
public:
	UnsharpRadius(UnsharpMain *plugin, int x, int y);
	int handle_event();
	UnsharpMain *plugin;
};

class UnsharpAmount : public BC_FPot
{
public:
	UnsharpAmount(UnsharpMain *plugin, int x, int y);
	int handle_event();
	UnsharpMain *plugin;
};

class UnsharpThreshold : public BC_IPot
{
public:
	UnsharpThreshold(UnsharpMain *plugin, int x, int y);
	int handle_event();
	UnsharpMain *plugin;
};

class UnsharpWindow : public BC_Window
{
public:
	UnsharpWindow(UnsharpMain *plugin, int x, int y);
	~UnsharpWindow();

	int create_objects();
	int close_event();
	void update();

	UnsharpRadius *radius;
	UnsharpAmount *amount;
	UnsharpThreshold *threshold;
	UnsharpMain *plugin;
};



PLUGIN_THREAD_HEADER(UnsharpMain, UnsharpThread, UnsharpWindow)




#endif

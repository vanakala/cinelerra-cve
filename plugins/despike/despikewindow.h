
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

#ifndef DESPIKEWINDOW_H
#define DESPIKEWINDOW_H

class DespikeThread;
class DespikeWindow;

#include "despike.h"
#include "guicast.h"
#include "mutex.h"
#include "pluginclient.h"

PLUGIN_THREAD_HEADER(Despike, DespikeThread, DespikeWindow)

class DespikeLevel;
class DespikeSlope;

class DespikeWindow : public BC_Window
{
public:
	DespikeWindow(Despike *despike, int x, int y);
	~DespikeWindow();
	
	int create_objects();
	int close_event();
	
	Despike *despike;
	DespikeLevel *level;
	DespikeSlope *slope;
};

class DespikeLevel : public BC_FSlider
{
public:
	DespikeLevel(Despike *despike, int x, int y);
	int handle_event();
	Despike *despike;
};

class DespikeSlope : public BC_FSlider
{
public:
	DespikeSlope(Despike *despike, int x, int y);
	int handle_event();
	Despike *despike;
};




#endif

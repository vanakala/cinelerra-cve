
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

#ifndef GAINWINDOW_H
#define GAINWINDOW_H

#define TOTAL_LOADS 5

class GainThread;
class GainWindow;

#include "filexml.h"
#include "gain.h"
#include "guicast.h"
#include "mutex.h"
#include "pluginclient.h"

PLUGIN_THREAD_HEADER(Gain, GainThread, GainWindow)

class GainLevel;

class GainWindow : public BC_Window
{
public:
	GainWindow(Gain *gain, int x, int y);
	~GainWindow();
	
	int create_objects();
	int close_event();
	
	Gain *gain;
	GainLevel *level;
};

class GainLevel : public BC_FSlider
{
public:
	GainLevel(Gain *gain, int x, int y);
	int handle_event();
	Gain *gain;
};




#endif

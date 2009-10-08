
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

#ifndef AGINGWINDOW_H
#define AGINGWINDOW_H

#include "guicast.h"

class AgingThread;
class AgingWindow;

#include "filexml.h"
#include "mutex.h"
#include "aging.h"

PLUGIN_THREAD_HEADER(AgingMain, AgingThread, AgingWindow)

class AgingColor;
class AgingScratches;
class AgingScratchCount;
class AgingPits;
class AgingPitCount;
class AgingDust;
class AgingDustCount;

class AgingWindow : public BC_Window
{
public:
	AgingWindow(AgingMain *client, int x, int y);
	~AgingWindow();

	int create_objects();
	int close_event();

	AgingMain *client;
	
	
	AgingColor *color;
	AgingScratches *scratches;
	AgingScratchCount *scratch_count;
	AgingPits *pits;
	AgingPitCount *pit_count;
	AgingDust *dust;
	AgingDustCount *dust_count;
};





class AgingColor : public BC_CheckBox
{
public:
	AgingColor(int x, int y, AgingMain *plugin);
	int handle_event();
	AgingMain *plugin;
};


class AgingScratches : public BC_CheckBox
{
public:
	AgingScratches(int x, int y, AgingMain *plugin);
	int handle_event();
	AgingMain *plugin;
};


class AgingScratchCount : public BC_ISlider
{
public:
	AgingScratchCount(int x, int y, AgingMain *plugin);
	int handle_event();
	AgingMain *plugin;
};

class AgingPits : public BC_CheckBox
{
public:
	AgingPits(int x, int y, AgingMain *plugin);
	int handle_event();
	AgingMain *plugin;
};

class AgingPitCount : public BC_ISlider
{
public:
	AgingPitCount(int x, int y, AgingMain *plugin);
	int handle_event();
	AgingMain *plugin;
};








class AgingDust : public BC_CheckBox
{
public:
	AgingDust(int x, int y, AgingMain *plugin);
	int handle_event();
	AgingMain *plugin;
};

class AgingDustCount : public BC_ISlider
{
public:
	AgingDustCount(int x, int y, AgingMain *plugin);
	int handle_event();
	AgingMain *plugin;
};







#endif

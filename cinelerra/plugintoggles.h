
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

#ifndef PLUGINTOGGLES_H
#define PLUGINTOGGLES_H


#include "guicast.h"
#include "mwindow.inc"
#include "plugin.inc"


class PluginOn : public BC_Toggle
{
public:
	PluginOn(MWindow *mwindow, int x, int y, Plugin *plugin);
	static int calculate_w(MWindow *mwindow);
	void update(int x, int y, Plugin *plugin);
	int handle_event();
	MWindow *mwindow;
	int in_use;
	Plugin *plugin;
};



class PluginShow : public BC_Toggle
{
public:
	PluginShow(MWindow *mwindow, int x, int y, Plugin *plugin);
	MWindow *mwindow;
	static int calculate_w(MWindow *mwindow);
	void update(int x, int y, Plugin *plugin);
	int handle_event();
	int in_use;
	Plugin *plugin;
};




#endif

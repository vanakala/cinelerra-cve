
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

#ifndef BANDWIPE_H
#define BANDWIPE_H

class BandWipeMain;
class BandWipeWindow;


#include "overlayframe.inc"
#include "pluginvclient.h"
#include "vframe.inc"




class BandWipeCount : public BC_TumbleTextBox
{
public:
	BandWipeCount(BandWipeMain *plugin, 
		BandWipeWindow *window,
		int x,
		int y);
	int handle_event();
	BandWipeMain *plugin;
	BandWipeWindow *window;
};


class BandWipeWindow : public BC_Window
{
public:
	BandWipeWindow(BandWipeMain *plugin, int x, int y);
	void create_objects();
	void close_event();
	BandWipeMain *plugin;
	BandWipeCount *count;
};


PLUGIN_THREAD_HEADER(BandWipeMain, BandWipeThread, BandWipeWindow)


class BandWipeMain : public PluginVClient
{
public:
	BandWipeMain(PluginServer *server);
	~BandWipeMain();

	PLUGIN_CLASS_MEMBERS(int, BandWipeThread);
	int process_realtime(VFrame *incoming, VFrame *outgoing);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int uses_gui();
	int is_transition();
	int is_video();

	int bands;
	int direction;
};

#endif

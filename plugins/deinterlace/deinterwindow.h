
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

#ifndef DEINTERWINDOW_H
#define DEINTERWINDOW_H


class DeInterlaceThread;
class DeInterlaceWindow;

#include "guicast.h"
#include "mutex.h"
#include "deinterlace.h"
#include "pluginclient.h"

PLUGIN_THREAD_HEADER(DeInterlaceMain, DeInterlaceThread, DeInterlaceWindow);

class DeInterlaceOption;
class DeInterlaceMode;
class DeInterlaceDominanceTop;
class DeInterlaceDominanceBottom;
class DeInterlaceAdaptive;
class DeInterlaceThreshold;

class DeInterlaceWindow : public BC_Window
{
public:
	DeInterlaceWindow(DeInterlaceMain *client, int x, int y);
	~DeInterlaceWindow();
	
	int create_objects();
	int close_event();
	int set_mode(int mode, int recursive);
	void get_status_string(char *string, int changed_rows);
	DeInterlaceMain *client;
	DeInterlaceMode *mode;
	DeInterlaceAdaptive *adaptive;
	DeInterlaceDominanceTop *dominance_top;
	DeInterlaceDominanceBottom *dominance_bottom;
	DeInterlaceThreshold *threshold;
	int optional_controls_x,optional_controls_y;
	BC_Title *status;
};

class DeInterlaceOption : public BC_Radial
{
public:
	DeInterlaceOption(DeInterlaceMain *client, 
		DeInterlaceWindow *window, 
		int output, 
		int x, 
		int y, 
		char *text);
	~DeInterlaceOption();
	int handle_event();

	DeInterlaceMain *client;
	DeInterlaceWindow *window;
	int output;
};

class DeInterlaceAdaptive : public BC_CheckBox
{
public:
	DeInterlaceAdaptive(DeInterlaceMain *client, int x, int y);
	int handle_event();
	DeInterlaceMain *client;
};

class DeInterlaceDominanceTop : public BC_Radial
{
public:
	DeInterlaceDominanceTop(DeInterlaceMain *client, DeInterlaceWindow *window, int x, int y, char * title);
	int handle_event();
	DeInterlaceMain *client;
	DeInterlaceWindow *window;

};
class DeInterlaceDominanceBottom : public BC_Radial
{
public:
	DeInterlaceDominanceBottom(DeInterlaceMain *client, DeInterlaceWindow *window, int x, int y, char * title);
	int handle_event();
	DeInterlaceMain *client;
	DeInterlaceWindow *window;
};

class DeInterlaceThreshold : public BC_IPot
{
public:
	DeInterlaceThreshold(DeInterlaceMain *client, int x, int y);
	~DeInterlaceThreshold();

	int handle_event();
	DeInterlaceMain *client;
	BC_Title *title_caption;
};



class DeInterlaceMode : public BC_PopupMenu
{
public:
	DeInterlaceMode(DeInterlaceMain *client, 
		DeInterlaceWindow *window, 
		int x, 
		int y);
	void create_objects();
	static char* to_text(int shape);
	static int from_text(char *text);
	int handle_event();
	DeInterlaceMain *plugin;
	DeInterlaceWindow *gui;
};


#endif


/*
 * CINELERRA
 * Copyright (C) 2004 Andraz Tori
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

#ifndef MANUALGOTO_H
#define MANUALGOTO_H

#include "awindow.inc"
#include "edl.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "thread.h"
#include "vwindow.inc"
#include "editpanel.inc"

class ManualGotoWindow;
class ManualGotoNumber;

class ManualGoto : public Thread
{
public:
	ManualGoto(MWindow *mwindow, BC_WindowBase *masterwindow);
	~ManualGoto();

	void run();

// If it is being created or edited
	MWindow *mwindow;
	BC_WindowBase *masterwindow;
	void open_window();

	ManualGotoWindow *gotowindow;
	int done;

};




class ManualGotoWindow : public BC_Window
{
public:
	ManualGotoWindow(MWindow *mwindow, ManualGoto *thread);
	~ManualGotoWindow();

	void create_objects();
	int activate();
	void reset_data(double position);
	double get_entered_position_sec();
	void set_entered_position_sec(double position);



// Use this copy of the pointer in ManualGoto since multiple windows are possible	
	BC_Title *signtitle;
	ManualGotoNumber *boxhours;
	ManualGotoNumber *boxminutes;
	ManualGotoNumber *boxseconds;
	ManualGotoNumber *boxmsec;
	MWindow *mwindow;
	ManualGoto *thread;
};



class ManualGotoNumber : public BC_TextBox
{
public:
	ManualGotoNumber(ManualGotoWindow *window, int x, int y, int w, int min_num, int max_num, int chars);
	int handle_event();
	ManualGotoWindow *window;
	int keypress_event();
	int activate();
	int deactivate();
	void reshape_update(int64_t number);
	
	int min_num;
	int max_num;
	int chars;
};







#endif

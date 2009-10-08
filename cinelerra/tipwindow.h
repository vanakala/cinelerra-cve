
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

#ifndef TIPWINDOW_H
#define TIPWINDOW_H

#include "bcdialog.h"
#include "guicast.h"
#include "mwindow.inc"
#include "tipwindow.inc"



// Tip of the day to be run at initialization


class TipWindow : public BC_DialogThread
{
public:
	TipWindow(MWindow *mwindow);

	BC_Window* new_gui();
	char* get_current_tip();
	void next_tip();
	void prev_tip();

	MWindow *mwindow;
	TipWindowGUI *gui;
};


class TipWindowGUI : public BC_Window
{
public:
	TipWindowGUI(MWindow *mwindow, 
		TipWindow *thread,
		int x,
		int y);
	void create_objects();
	int keypress_event();
	MWindow *mwindow;
	TipWindow *thread;
	BC_Title *tip_text;
};

class TipDisable : public BC_CheckBox
{
public:
	TipDisable(MWindow *mwindow, TipWindowGUI *gui, int x, int y);
	int handle_event();
	TipWindowGUI *gui;
	MWindow *mwindow;
};

class TipNext : public BC_Button
{
public:
	TipNext(MWindow *mwindow, TipWindowGUI *gui, int x, int y);
	int handle_event();
	static int calculate_w(MWindow *mwindow);
	TipWindowGUI *gui;
	MWindow *mwindow;
};

class TipPrev : public BC_Button
{
public:
	TipPrev(MWindow *mwindow, TipWindowGUI *gui, int x, int y);
	int handle_event();
	static int calculate_w(MWindow *mwindow);
	TipWindowGUI *gui;
	MWindow *mwindow;
};

class TipClose : public BC_Button
{
public:
	TipClose(MWindow *mwindow, TipWindowGUI *gui, int x, int y);
	int handle_event();
	static int calculate_w(MWindow *mwindow);
	static int calculate_h(MWindow *mwindow);
	TipWindowGUI *gui;
	MWindow *mwindow;
};



#endif

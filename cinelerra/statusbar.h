
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

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include "bcbutton.h"
#include "bcsubwindow.h"
#include "mwindow.inc"
#include "mwindowgui.inc"

class StatusBarCancel;

class StatusBar : public BC_SubWindow
{
public:
	StatusBar(MWindow *mwindow, MWindowGUI *gui);

	void set_message(const char *text);
	void show();
	void resize_event();

	MWindow *mwindow;
	MWindowGUI *gui;
	BC_ProgressBar *main_progress;
	StatusBarCancel *main_progress_cancel;
	BC_Title *status_text;
};

class StatusBarCancel : public BC_Button
{
public:
	StatusBarCancel(MWindow *mwindow, int x, int y);

	int handle_event();

	MWindow *mwindow;
};

#endif


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

#ifndef BROWSEBUTTON_H
#define BROWSEBUTTON_H

#include "guicast.h"
#include "mutex.inc"
#include "mwindow.inc"
#include "thread.h"

class BrowseButtonWindow;

class BrowseButton : public BC_Button, public Thread
{
public:
	BrowseButton(MWindow *mwindow, 
		BC_WindowBase *parent_window, 
		BC_TextBox *textbox, 
		int x, 
		int y, 
		const char *init_directory, 
		const char *title, 
		const char *caption, 
		int want_directory = 0,
		const char *recent_prefix = NULL);
	~BrowseButton();
	
	int handle_event();
	void run();
	int want_directory;
	char result[1024];
	const char *title;
	const char *caption;
	const char *init_directory;
	BC_TextBox *textbox;
	MWindow *mwindow;
	BC_WindowBase *parent_window;
	BrowseButtonWindow *gui;
	Mutex *startup_lock;
	int x, y;
	const char *recent_prefix;
};

class BrowseButtonWindow : public BC_FileBox
{
public:
	BrowseButtonWindow(MWindow *mwindow,
		BrowseButton *button,
		BC_WindowBase *parent_window, 
		const char *init_directory, 
		const char *title, 
		const char *caption, 
		int want_directory);
	~BrowseButtonWindow();
};





#endif

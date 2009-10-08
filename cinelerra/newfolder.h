
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

#ifndef NEWFOLDER_H
#define NEWFOLDER_H

#include "awindowgui.inc"
#include "guicast.h"
#include "mutex.h"
#include "mwindow.inc"

class NewFolder : public BC_Window
{
public:
	NewFolder(MWindow *mwindow, AWindowGUI *awindow, int x, int y);
	~NewFolder();

	int create_objects();
	char* get_text();

private:
	BC_TextBox *textbox;
	MWindow *mwindow;
	AWindowGUI *awindow;
};


class NewFolderThread : public Thread
{
public:
	NewFolderThread(MWindow *mwindow, AWindowGUI *awindow);
	~NewFolderThread();

	void run();
	int interrupt();
	int start_new_folder();

private:
	Mutex change_lock, completion_lock;
	int active;
	MWindow *mwindow;
	AWindowGUI *awindow;
	NewFolder *window;
};

#endif

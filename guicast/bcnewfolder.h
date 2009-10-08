
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

#ifndef BC_NEWFOLDER_H
#define BC_NEWFOLDER_H


#include "bcfilebox.inc"
#include "bcwindow.h"
#include "thread.h"


class BC_NewFolder : public BC_Window
{
public:
	BC_NewFolder(int x, int y, BC_FileBox *filebox);
	~BC_NewFolder();

	int create_objects();
	char* get_text();

private:
	BC_TextBox *textbox;
};

class BC_NewFolderThread : public Thread
{
public:
	BC_NewFolderThread(BC_FileBox *filebox);
	~BC_NewFolderThread();

	void run();
	int interrupt();
	int start_new_folder();

private:
	Mutex *change_lock;
	Condition *completion_lock;
	BC_FileBox *filebox;
	BC_NewFolder *window;
};





#endif

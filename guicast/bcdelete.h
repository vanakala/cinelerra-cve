
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

#ifndef BCDELETE_H
#define BCDELETE_H



#include "bcdialog.h"
#include "bcfilebox.inc"

class BC_DeleteFile : public BC_Window
{
public:
	BC_DeleteFile(BC_FileBox *filebox, int x, int y);
	~BC_DeleteFile();
	void create_objects();
	BC_FileBox *filebox;
	ArrayList<BC_ListBoxItem*> *data;
};

class BC_DeleteList : public BC_ListBox
{
public:
	BC_DeleteList(BC_FileBox *filebox, 
		int x, 
		int y, 
		int w, 
		int h,
		ArrayList<BC_ListBoxItem*> *data);
	BC_FileBox *filebox;
};

class BC_DeleteThread : public BC_DialogThread
{
public:
	BC_DeleteThread(BC_FileBox *filebox);
	void handle_done_event(int result);
	BC_Window* new_gui();

	BC_FileBox *filebox;
};








#endif

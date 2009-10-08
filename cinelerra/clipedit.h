
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

#ifndef CLIPEDIT_H
#define CLIPEDIT_H

#include "awindow.inc"
#include "edl.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "thread.h"
#include "vwindow.inc"


class ClipEdit : public Thread
{
public:
	ClipEdit(MWindow *mwindow, AWindow *awindow, VWindow *vwindow);
	~ClipEdit();

	void run();
	void edit_clip(EDL *clip);
	void create_clip(EDL *clip);

// If it is being created or edited
	MWindow *mwindow;
	AWindow *awindow;
	VWindow *vwindow;


	EDL *clip;
	int create_it;
};




class ClipEditWindow : public BC_Window
{
public:
	ClipEditWindow(MWindow *mwindow, ClipEdit *thread);
	~ClipEditWindow();

	void create_objects();


// Use this copy of the pointer in ClipEdit since multiple windows are possible	
	EDL *clip;
	int create_it;
	MWindow *mwindow;
	ClipEdit *thread;
	BC_TextBox *titlebox;
};



class ClipEditTitle : public BC_TextBox
{
public:
	ClipEditTitle(ClipEditWindow *window, int x, int y, int w);
	int handle_event();
	ClipEditWindow *window;
};


class ClipEditComments : public BC_TextBox
{
public:
	ClipEditComments(ClipEditWindow *window, int x, int y, int w, int rows);
	int handle_event();
	ClipEditWindow *window;
};






#endif

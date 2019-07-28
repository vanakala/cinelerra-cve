
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

#include "bctextbox.h"
#include "bcwindow.h"
#include "edl.inc"
#include "localsession.inc"
#include "thread.h"

// Multiple windows of ClipEdit can be opened
class ClipEdit : public Thread
{
public:
	ClipEdit();

// Modifies clip title and notes
	void edit_clip(EDL *edl);
// edl will be inserted to ClipList or deleted
	void create_clip(EDL *edl);

private:
	void run();
	LocalSession *session;
	EDL *edl;
};


class ClipEditWindow : public BC_Window
{
public:
	ClipEditWindow(LocalSession *session, EDL *edl,
		int absx, int absy);

	char clip_title[BCTEXTLEN];
	char clip_notes[BCTEXTLEN];
	BC_TextBox *titlebox;
	EDL *edl;
	LocalSession *session;
};


class ClipEditTitle : public BC_TextBox
{
public:
	ClipEditTitle(ClipEditWindow *window, int x, int y, int w);

private:
	int handle_event();

	ClipEditWindow *window;
};


class ClipEditComments : public BC_TextBox
{
public:
	ClipEditComments(ClipEditWindow *window, int x, int y, int w, int rows);

private:
	int handle_event();

	ClipEditWindow *window;
};

#endif

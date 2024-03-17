// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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

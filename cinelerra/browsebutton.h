// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BROWSEBUTTON_H
#define BROWSEBUTTON_H

#include "bcbutton.h"
#include "bctextbox.h"
#include "bcfilebox.h"
#include "browsebutton.inc"
#include "mutex.inc"
#include "thread.h"

class BrowseButtonWindow;

class BrowseButton : public BC_Button, public Thread
{
public:
	BrowseButton(BC_WindowBase *parent_window, 
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
	const char *title;
	const char *caption;
	const char *init_directory;
	BC_TextBox *textbox;
	BC_WindowBase *parent_window;
	BrowseButtonWindow *gui;
	Mutex *startup_lock;
	int x, y;
	const char *recent_prefix;
};

class BrowseButtonWindow : public BC_FileBox
{
public:
	BrowseButtonWindow(BrowseButton *button,
		BC_WindowBase *parent_window, 
		const char *init_directory, 
		const char *title, 
		const char *caption, 
		int want_directory);
};

#endif

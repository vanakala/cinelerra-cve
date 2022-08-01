// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BCDIALOG_H
#define BCDIALOG_H

#include "bcdialog.inc"
#include "bcwindow.inc"
#include "condition.inc"
#include "mutex.inc"
#include "thread.h"

// Generic dialog box with static thread and proper locking.
// Create the thread object at startup.  Call start() to do the dialog.
// Only one dialog instance is allowed at a time.  These must be overridden
// to add functionality.

class BC_DialogThread : public Thread
{
public:
	BC_DialogThread();
	virtual ~BC_DialogThread();

// User calls this to create or raise the dialog box.
	void start();
	void run();

// After the window is closed, this is called
	virtual void handle_done_event(int result);

// After the window is closed and deleted, this is called.
	virtual void handle_close_event(int result);

// User creates the window and initializes it here.
	virtual BC_Window* new_gui();
	BC_Window* get_gui();

// Called by user to access the gui pointer
	void lock_window(const char *location);
	void unlock_window();

private:
	BC_Window *gui;
	Condition *startup_lock;
	Mutex *window_lock;
};








#endif

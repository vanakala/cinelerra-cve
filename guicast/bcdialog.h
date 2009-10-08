
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

#ifndef BCDIALOG_H
#define BCDIALOG_H

#include "bcdialog.inc"
#include "condition.inc"
#include "guicast.h"
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
	void lock_window(char *location);
	void unlock_window();

private:
	BC_Window *gui;
	Condition *startup_lock;
	Mutex *window_lock;
};








#endif

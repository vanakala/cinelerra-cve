// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcdialog.h"
#include "bcwindow.h"
#include "condition.h"
#include "mutex.h"

BC_DialogThread::BC_DialogThread()
 : Thread(THREAD_SYNCHRONOUS)
{
	gui = 0;
	startup_lock = new Condition(1, "BC_DialogThread::startup_lock");
	window_lock = new Mutex("BC_DialogThread::window_lock");
}

BC_DialogThread::~BC_DialogThread()
{
	startup_lock->lock("BC_DialogThread::~BC_DialogThread");
	if(gui)
	{
		gui->lock_window("BC_DialogThread::Destructor");
		gui->set_done(1);
		gui->unlock_window();
	}
	startup_lock->unlock();
	Thread::join();

	delete startup_lock;
	delete window_lock;
}

void BC_DialogThread::lock_window(const char *location)
{
	window_lock->lock(location);
}

void BC_DialogThread::unlock_window()
{
	window_lock->unlock();
}

void BC_DialogThread::start()
{
	if(Thread::running())
	{
		window_lock->lock("BC_DialogThread::start");
		if(gui)
		{
			gui->lock_window("BC_DialogThread::start");
			gui->raise_window();
			gui->unlock_window();
		}
		window_lock->unlock();
		return;
	}

// Don't allow anyone else to create the window
	startup_lock->lock("BC_DialogThread::start");
	Thread::start();

// Wait for startup
	startup_lock->lock("BC_DialogThread::start");
	startup_lock->unlock();
}

void BC_DialogThread::run()
{
	gui = new_gui();
	startup_lock->unlock();
	int result = gui->run_window();

	handle_done_event(result);

	window_lock->lock("BC_DialogThread::run");
	delete gui;
	gui = 0;
	window_lock->unlock();

	handle_close_event(result);
}

BC_Window* BC_DialogThread::new_gui()
{
	printf("BC_DialogThread::new_gui called\n");
	return 0;
}

BC_Window* BC_DialogThread::get_gui()
{
	return gui;
}

void BC_DialogThread::handle_done_event(int result)
{
}

void BC_DialogThread::handle_close_event(int result)
{
}





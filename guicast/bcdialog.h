#ifndef BCDIALOG_H
#define BCDIALOG_H

#include "bcdialog.inc"
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

	void start();
	void run();

// After the window is closed
	virtual void handle_close_event(int result);

// Create the window and initialize it.
	virtual BC_Window* new_gui();

private:
	BC_Window *gui;
	Mutex *startup_lock;
	Mutex *window_lock;
};








#endif

#ifndef BCWINDOWEVENTS_H
#define BCWINDOWEVENTS_H

// Thread running parallel to run_window which listens for X server events 
// only.  Hopefully, having it as the only thing calling the X server without 
// locks won't crash.  Previously, events were dispatched from BC_Repeater 
// asynchronous to the events recieved by XNextEvent.  BC_Repeater tended to
// lock up for no reason.


#include "bcwindowbase.inc"
#include "thread.h"



class BC_WindowEvents : public Thread
{
public:
	BC_WindowEvents(BC_WindowBase *window);
	~BC_WindowEvents();
	void start();
	void run();
	BC_WindowBase *window;
	int done;
};




#endif

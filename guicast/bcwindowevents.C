#include "bcwindowbase.h"
#include "bcwindowevents.h"

BC_WindowEvents::BC_WindowEvents(BC_WindowBase *window)
 : Thread(1, 0, 0)
{
	this->window = window;
	done = 0;
}

BC_WindowEvents::~BC_WindowEvents()
{
	done = 1;
	Thread::join();
}

void BC_WindowEvents::start()
{
	done = 0;
	Thread::start();
}


void BC_WindowEvents::run()
{
	XEvent *event;
	while(!done)
	{
		event = new XEvent;
// Can't cancel in XNextEvent because X server never figures out it's not
// listening anymore and XCloseDisplay locks up.
		XNextEvent(window->display, event);
		window->put_event(event);
	}
}




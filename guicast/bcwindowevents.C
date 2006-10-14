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
// First set done, then send dummy event through XSendEvent to unlock the loop in ::run()
	done = 1;
	XEvent event;
	XClientMessageEvent *ptr = (XClientMessageEvent*)&event;
	event.type = ClientMessage;
	ptr->message_type = XInternAtom(window->display, "DUMMY_XATOM", False);
	ptr->format = 32;
	XSendEvent(window->display, 
		window->win, 
		0, 
		0, 
		&event);
	window->flush();
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




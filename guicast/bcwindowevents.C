#include "bcwindowbase.h"
#include "bcwindowevents.h"
#include "bctimer.h"

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
// Can't cancel in XNextEvent because X server never figures out it's not
// listening anymore and XCloseDisplay locks up.
	XEvent *event;
	while(!done)
	{
// XNextEvent should also be protected by XLockDisplay ...
// Currently implemented is a hackish solution, FIXME
// Impact of this solution on the performance has not been analyzed

// The proper solution is HARD because of :
// 1. idiotic xlib does not have XTryLockDisplay - we will need to _completely_ implement XLockDisplay by ourselves (including cascaded locking - it is not the same as mutex!)
// 2. the locking shemantic inside new lock_window and here will be really tricky, we should:
//	in lock_window check wheather BC_WindowEvents is in XNextEvent and it is send custom xevent to break out of the loop and make sure lock is not taken again if lock_window() is waiting on it
// 3. Send custom events from previous point through _separate_ xwindows display connection since XSendEvent would need to be protected by XLockDisplay which obviously can't be

		window->lock_window();
		while (XPending(window->display))
		{
			event = new XEvent;
			XNextEvent(window->display, event);
			window->put_event(event);
		}
		
		window->unlock_window();
		Timer::delay(20);    // sleep 20ms
	}
}




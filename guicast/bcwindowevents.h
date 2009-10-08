
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


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

#include "bcclipboard.h"
#include "bcwindowbase.h"
#include <string.h>

BC_Clipboard::BC_Clipboard(char *display_name) : Thread()
{
	Thread::set_synchronous(1);

	in_display = BC_WindowBase::init_display(display_name);
	out_display = BC_WindowBase::init_display(display_name);
	completion_atom = XInternAtom(out_display, "BC_CLOSE_EVENT", False);
	primary = XA_PRIMARY;
	secondary = XInternAtom(out_display, "CLIPBOARD", False);
	targets_atom = XInternAtom(out_display, "TARGETS", False);
	in_win = XCreateSimpleWindow(in_display, 
				DefaultRootWindow(in_display), 
				0, 
				0, 
				1, 
				1, 
				0,
				0,
				0);
	out_win = XCreateSimpleWindow(out_display, 
				DefaultRootWindow(out_display), 
				0, 
				0, 
				1, 
				1, 
				0,
				0,
				0);
	data[0] = 0;
	data[1] = 0;
}

BC_Clipboard::~BC_Clipboard()
{
	if(data[0]) delete [] data[0];
	if(data[1]) delete [] data[1];

	XDestroyWindow(in_display, in_win);
	XCloseDisplay(in_display);
	XDestroyWindow(out_display, out_win);
	XCloseDisplay(out_display);
}

int BC_Clipboard::start_clipboard()
{
	Thread::start();
	return 0;
}

int BC_Clipboard::stop_clipboard()
{
	XEvent event;
	XClientMessageEvent *ptr = (XClientMessageEvent*)&event;

	event.type = ClientMessage;
	ptr->message_type = completion_atom;
	ptr->format = 32;
	XSendEvent(out_display, out_win, 0, 0, &event);
	XFlush(out_display);
	Thread::join();
	return 0;
}

void BC_Clipboard::run()
{
	XEvent event;
	XClientMessageEvent *ptr;
	int done = 0;

	while(!done)
	{
//printf("BC_Clipboard::run 1\n");					
		XNextEvent(out_display, &event);
//printf("BC_Clipboard::run 2 %d\n", event.type);					

		XLockDisplay(out_display);
		switch(event.type)
		{
// Termination signal
			case ClientMessage:
				ptr = (XClientMessageEvent*)&event;
				if(ptr->message_type == completion_atom)
				{
					done = 1;
				}
//printf("ClientMessage %x %x %d\n", ptr->message_type, ptr->data.l[0], primary_atom);
				break;


			case SelectionRequest:
				handle_selectionrequest((XSelectionRequestEvent*)&event);
				break;
			
			
			
			case SelectionClear:
				if(data[0]) data[0][0] = 0;
				if(data[1]) data[1][0] = 0;
				break;
		}
		XUnlockDisplay(out_display);
	}
}

void BC_Clipboard::handle_selectionrequest(XSelectionRequestEvent *request)
{
	int success = 0;
	if (request->target == XA_STRING)
		success = handle_request_string(request);
	else if (request->target == targets_atom)
		success = handle_request_targets(request);

	XEvent reply;
// 'None' tells the client that the request was denied
	reply.xselection.property  = success ? request->property : None;
	reply.xselection.type      = SelectionNotify;
	reply.xselection.display   = request->display;
	reply.xselection.requestor = request->requestor;
	reply.xselection.selection = request->selection;
	reply.xselection.target    = request->target;
	reply.xselection.time      = request->time;
					

	XSendEvent(out_display, request->requestor, 0, 0, &reply);
	XFlush(out_display);
//printf("SelectionRequest\n");
}

int BC_Clipboard::handle_request_string(XSelectionRequestEvent *request)
{
	char *data_ptr = (request->selection == primary ? data[0] : data[1]);

	XChangeProperty(out_display,
			request->requestor,
			request->property,
			XA_STRING,
			8,
			PropModeReplace,
			(unsigned char*)data_ptr,
			strlen(data_ptr));
	return 1;
}

int BC_Clipboard::handle_request_targets(XSelectionRequestEvent *request)
{
	Atom targets[] = {
		targets_atom,
		XA_STRING
	};
	XChangeProperty(out_display,
			request->requestor,
			request->property,
			XA_ATOM,
			32,
			PropModeReplace,
			(unsigned char*)targets,
			sizeof(targets)/sizeof(targets[0]));
//printf("BC_Clipboard::handle_request_targets\n");
	return 1;
}

int BC_Clipboard::to_clipboard(char *data, long len, int clipboard_num)
{

#if 0
	XStoreBuffer(display, data, len, clipboard_num);
#endif

	XLockDisplay(out_display);

// Store in local buffer
	if(this->data[clipboard_num] && length[clipboard_num] != len + 1)
	{
		delete [] this->data[clipboard_num];
		this->data[clipboard_num] = 0;
	}

	if(!this->data[clipboard_num])
	{
		length[clipboard_num] = len;
		this->data[clipboard_num] = new char[len + 1];
		memcpy(this->data[clipboard_num], data, len);
		this->data[clipboard_num][len] = 0;
	}

	XSetSelectionOwner(out_display, 
		(clipboard_num == PRIMARY_SELECTION) ? primary : secondary, 
		out_win, 
		CurrentTime);

	XFlush(out_display);

	XUnlockDisplay(out_display);
	return 0;
}

int BC_Clipboard::from_clipboard(char *data, long maxlen, int clipboard_num)
{



#if 0
	char *data2;
	int len, i;
	data2 = XFetchBuffer(display, &len, clipboard_num);
	for(i = 0; i < len && i < maxlen; i++)
		data[i] = data2[i];

	data[i] = 0;

	XFree(data2);

#endif


	XLockDisplay(in_display);

	XEvent event;
    Atom type_return, pty;
    int format;
    unsigned long nitems, size, new_size, total;
	char *temp_data = 0;

    pty = (clipboard_num == PRIMARY_SELECTION) ? primary : secondary; 
						/* a property of our window
						   for apps to put their
						   selection into */

	XConvertSelection(in_display, 
		clipboard_num == PRIMARY_SELECTION ? primary : secondary, 
		XA_STRING, 
		pty,
       	in_win, 
		CurrentTime);

	data[0] = 0;
	do
	{
		XNextEvent(in_display, &event);
	}while(event.type != SelectionNotify && event.type != None);

	if(event.type != None)
	{
// Get size
   	XGetWindowProperty(in_display,
        	in_win,
        	pty,
        	0,
        	0,
        	False,
        	AnyPropertyType,
        	&type_return,
        	&format,
        	&nitems,
        	&size,
        	(unsigned char**)&temp_data);

	if(temp_data) XFree(temp_data);
	temp_data = 0;

// Get data
   	XGetWindowProperty(in_display,
        	in_win,
        	pty,
        	0,
        	size,
        	False,
        	AnyPropertyType,
        	&type_return,
        	&format,
        	&nitems,
        	&new_size,
        	(unsigned char**)&temp_data);


		if(type_return && temp_data)
		{
			strncpy(data, temp_data, maxlen);
			data[size] = 0;
		}
		else
			data[0] = 0;

		if(temp_data) XFree(temp_data);
	}

	XUnlockDisplay(in_display);

	return 0;
}

long BC_Clipboard::clipboard_len(int clipboard_num)
{

#if 0
	char *data2;
	int len;

	data2 = XFetchBuffer(display, &len, clipboard_num);
	XFree(data2);
	return len;
#endif



	XLockDisplay(in_display);

	XEvent event;
    Atom type_return, pty;
    int format;
    unsigned long nitems, pty_size, total;
	char *temp_data = 0;
	int result = 0;

    pty = (clipboard_num == PRIMARY_SELECTION) ? primary : secondary; 
						/* a property of our window
						   for apps to put their
						   selection into */
	XConvertSelection(in_display, 
		(clipboard_num == PRIMARY_SELECTION) ? primary : secondary, 
		XA_STRING, 
		pty,
       	in_win, 
		CurrentTime);

	do
	{
		XNextEvent(in_display, &event);
	}while(event.type != SelectionNotify && event.type != None);

	if(event.type != None)
	{
// Get size
    	XGetWindowProperty(in_display,
        	in_win,
        	pty,
        	0,
        	0,
        	False,
        	AnyPropertyType,
        	&type_return,
        	&format,
        	&nitems,
        	&pty_size,
        	(unsigned char**)&temp_data);

		if(type_return)
		{
			result = pty_size + 1;
		}
		else
			result = 0;



		if(temp_data)
			XFree(temp_data);

	}

	XUnlockDisplay(in_display);

	return result;
}

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
				{
					XEvent reply;
					XSelectionRequestEvent *request = (XSelectionRequestEvent*)&event;
					char *data_ptr = (request->selection == primary ? data[0] : data[1]);

//printf("BC_Clipboard::run 2\n");					
        			XChangeProperty(out_display,
        				request->requestor,
        				request->property,
        				XA_STRING,
        				8,
        				PropModeReplace,
        				(unsigned char*)data_ptr,
        				strlen(data_ptr));
					
        			reply.xselection.property  = request->property;
        			reply.xselection.type      = SelectionNotify;
        			reply.xselection.display   = request->display;
        			reply.xselection.requestor = request->requestor;
        			reply.xselection.selection = request->selection;
        			reply.xselection.target    = request->target;
        			reply.xselection.time      = request->time;
					

					XSendEvent(out_display, request->requestor, 0, 0, &reply);
					XFlush(out_display);
				}
//printf("SelectionRequest\n");
				break;
			
			
			
			case SelectionClear:
				if(data[0]) data[0][0] = 0;
				if(data[1]) data[1][0] = 0;
				break;
		}
		XUnlockDisplay(out_display);
	}
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
	char *temp_data;

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
	char *temp_data;
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
	}

	XUnlockDisplay(in_display);

	return result;
}

#include "bcdisplayinfo.h"
#include "blurzoomwindow.h"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)


BlurZoomThread::BlurZoomThread(BlurZoomMain *client)
 : Thread()
{
	this->client = client;
	set_synchronous(0);
	gui_started.lock();
	completion.lock();
}

BlurZoomThread::~BlurZoomThread()
{
// Window always deleted here
	delete window;
}
	
void BlurZoomThread::run()
{
	BC_DisplayInfo info;
	window = new BlurZoomWindow(client, 
		info.get_abs_cursor_x() - 105, 
		info.get_abs_cursor_y() - 100);
	window->create_objects();
	gui_started.unlock();
	int result = window->run_window();
	completion.unlock();
// Last command executed in thread
	if(result) client->client_side_close();
}






BlurZoomWindow::BlurZoomWindow(BlurZoomMain *client, int x, int y)
 : BC_Window(client->gui_string, 
	x,
	y,
	300, 
	170, 
	300, 
	170, 
	0, 
	0,
	1)
{ 
	this->client = client; 
}

BlurZoomWindow::~BlurZoomWindow()
{
}

int BlurZoomWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, 
		_("RadioacTV from EffectTV\n"
		"Copyright (C) 2001 FUKUCHI Kentarou")
	));

	show_window();
	flush();
	return 0;
}

int BlurZoomWindow::close_event()
{
// Set result to 1 to indicate a client side close
	set_done(1);
	return 1;
}



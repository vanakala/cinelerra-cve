#include "bcdisplayinfo.h"
#include "burnwindow.h"
#include "language.h"




PLUGIN_THREAD_OBJECT(BurnMain, BurnThread, BurnWindow)






BurnWindow::BurnWindow(BurnMain *client, int x, int y)
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

BurnWindow::~BurnWindow()
{
}

int BurnWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, 
		_("BurningTV from EffectTV\n"
		"Copyright (C) 2001 FUKUCHI Kentarou")
	));

	show_window();
	flush();
	return 0;
}

int BurnWindow::close_event()
{
// Set result to 1 to indicate a client side close
	set_done(1);
	return 1;
}



#include "bcdisplayinfo.h"
#include "dotwindow.h"
#include "language.h"
#include "pluginclient.h"

PLUGIN_THREAD_OBJECT(DotMain, DotThread, DotWindow)






DotWindow::DotWindow(DotMain *client, int x, int y)
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

DotWindow::~DotWindow()
{
}

int DotWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, 
		_("DotTV from EffectTV\n"
		"Copyright (C) 2001 FUKUCHI Kentarou")
	));

	show_window();
	flush();
	return 0;
}

WINDOW_CLOSE_EVENT(DotWindow)







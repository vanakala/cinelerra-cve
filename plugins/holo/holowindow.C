#include "bcdisplayinfo.h"
#include "language.h"
#include "holowindow.h"



PLUGIN_THREAD_OBJECT(HoloMain, HoloThread, HoloWindow)




HoloWindow::HoloWindow(HoloMain *client, int x, int y)
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

HoloWindow::~HoloWindow()
{
}

int HoloWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, 
		_("HolographicTV from EffectTV\n"
		"Copyright (C) 2001 FUKUCHI Kentarou")
	));

	show_window();
	flush();
	return 0;
}

WINDOW_CLOSE_EVENT(HoloWindow)






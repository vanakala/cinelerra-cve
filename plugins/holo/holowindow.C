#include "bcdisplayinfo.h"
#include "holowindow.h"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)


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

int HoloWindow::close_event()
{
// Set result to 1 to indicate a client side close
	set_done(1);
	return 1;
}







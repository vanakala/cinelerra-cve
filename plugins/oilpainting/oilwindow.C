
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

#include "oilwindow.h"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)


OilThread::OilThread(OilMain *client)
 : Thread()
{
	this->client = client;
	synchronous = 1; // make thread wait for join
	gui_started.lock();
}

OilThread::~OilThread()
{
}
	
void OilThread::run()
{
	window = new OilWindow(client);
	window->create_objects();
	gui_started.unlock();
	window->run_window();
	delete window;
}






OilWindow::OilWindow(OilMain *client)
 : BC_Window("", MEGREY, client->gui_string, 150, 130, 150, 130, 0, !client->show_initially)
{ this->client = client; }

OilWindow::~OilWindow()
{
	delete radius;
}

int OilWindow::create_objects()
{
	int x = 10, y = 10;
	add_tool(new BC_Title(x, y, _("Oil Painting")));
	y += 20;
	add_tool(radius = new OilRadius(client, x, y));
	x += 50;
	add_tool(new BC_Title(x, y, _("Radius")));
	y += 50;
	x = 10;
	add_tool(use_intensity = new OilIntensity(client, x, y));
}

int OilWindow::close_event()
{
	hide_window();
	client->send_hide_gui();
}

OilRadius::OilRadius(OilMain *client, int x, int y)
 : BC_IPot(x, y, 35, 35, client->radius, 1, 45, DKGREY, BLACK)
{
	this->client = client;
}
OilRadius::~OilRadius()
{
}
int OilRadius::handle_event()
{
	client->radius = get_value();
	client->send_configure_change();
}


OilIntensity::OilIntensity(OilMain *client, int x, int y)
 : BC_CheckBox(x, y, 16, 16, client->use_intensity, _("Use Intensity"))
{
	this->client = client;
}
OilIntensity::~OilIntensity()
{
}
int OilIntensity::handle_event()
{
	client->use_intensity = get_value();
	client->send_configure_change();
}




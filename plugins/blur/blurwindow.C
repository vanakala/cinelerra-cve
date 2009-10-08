
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

#include "bcdisplayinfo.h"
#include "blur.h"
#include "blurwindow.h"


#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

PLUGIN_THREAD_OBJECT(BlurMain, BlurThread, BlurWindow)





BlurWindow::BlurWindow(BlurMain *client, int x, int y)
 : BC_Window(client->gui_string, 
 	x,
	y,
	150, 
	270, 
	150, 
	270, 
	0, 
	1)
{ 
	this->client = client; 
}

BlurWindow::~BlurWindow()
{
//printf("BlurWindow::~BlurWindow 1\n");
}

int BlurWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, _("Blur")));
	y += 20;
	add_subwindow(horizontal = new BlurHorizontal(client, this, x, y));
	y += 30;
	add_subwindow(vertical = new BlurVertical(client, this, x, y));
	y += 35;
	add_subwindow(radius = new BlurRadius(client, x, y));
	add_subwindow(new BC_Title(x + 50, y, _("Radius")));
	y += 50;
	add_subwindow(a = new BlurA(client, x, y));
	y += 30;
	add_subwindow(r = new BlurR(client, x, y));
	y += 30;
	add_subwindow(g = new BlurG(client, x, y));
	y += 30;
	add_subwindow(b = new BlurB(client, x, y));
	
	show_window();
	flush();
	return 0;
}

int BlurWindow::close_event()
{
// Set result to 1 to indicate a client side close
	set_done(1);
	return 1;
}

BlurRadius::BlurRadius(BlurMain *client, int x, int y)
 : BC_IPot(x, 
 	y, 
	client->config.radius, 
	0, 
	MAXRADIUS)
{
	this->client = client;
}
BlurRadius::~BlurRadius()
{
}
int BlurRadius::handle_event()
{
	client->config.radius = get_value();
	client->send_configure_change();
	return 1;
}

BlurVertical::BlurVertical(BlurMain *client, BlurWindow *window, int x, int y)
 : BC_CheckBox(x, 
 	y, 
	client->config.vertical, 
	_("Vertical"))
{
	this->client = client;
	this->window = window;
}
BlurVertical::~BlurVertical()
{
}
int BlurVertical::handle_event()
{
	client->config.vertical = get_value();
	client->send_configure_change();
}

BlurHorizontal::BlurHorizontal(BlurMain *client, BlurWindow *window, int x, int y)
 : BC_CheckBox(x, 
 	y, 
	client->config.horizontal, 
	_("Horizontal"))
{
	this->client = client;
	this->window = window;
}
BlurHorizontal::~BlurHorizontal()
{
}
int BlurHorizontal::handle_event()
{
	client->config.horizontal = get_value();
	client->send_configure_change();
}




BlurA::BlurA(BlurMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.a, _("Blur alpha"))
{
	this->client = client;
}
int BlurA::handle_event()
{
	client->config.a = get_value();
	client->send_configure_change();
	return 1;
}

BlurR::BlurR(BlurMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.r, _("Blur red"))
{
	this->client = client;
}
int BlurR::handle_event()
{
	client->config.r = get_value();
	client->send_configure_change();
	return 1;
}

BlurG::BlurG(BlurMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.g, _("Blur green"))
{
	this->client = client;
}
int BlurG::handle_event()
{
	client->config.g = get_value();
	client->send_configure_change();
	return 1;
}

BlurB::BlurB(BlurMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.b, _("Blur blue"))
{
	this->client = client;
}
int BlurB::handle_event()
{
	client->config.b = get_value();
	client->send_configure_change();
	return 1;
}



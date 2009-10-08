
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

#include "yuvwindow.h"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)


YUVThread::YUVThread(YUVMain *client)
 : Thread()
{
	this->client = client;
	synchronous = 1; // make thread wait for join
	gui_started.lock();
}

YUVThread::~YUVThread()
{
}
	
void YUVThread::run()
{
	window = new YUVWindow(client);
	window->create_objects();
	gui_started.unlock();
	window->run_window();
	delete window;
}






YUVWindow::YUVWindow(YUVMain *client)
 : BC_Window("", MEGREY, client->gui_string, 210, 170, 200, 170, 0, !client->show_initially)
{ this->client = client; }

YUVWindow::~YUVWindow()
{
	delete y_slider;
	delete u_slider;
	delete v_slider;
	delete automation[0];
	delete automation[1];
	delete automation[2];
}

int YUVWindow::create_objects()
{
	int x = 10, y = 10;
	add_tool(new BC_Title(x, y, _("Y:")));
	add_tool(automation[0] = new AutomatedFn(client, this, x + 80, y, 0));
	y += 20;
	add_tool(y_slider = new YSlider(client, x, y));
	y += 35;
	add_tool(new BC_Title(x, y, _("U:")));
	add_tool(automation[1] = new AutomatedFn(client, this, x + 80, y, 1));
	y += 20;
	add_tool(u_slider = new USlider(client, x, y));
	y += 35;
	add_tool(new BC_Title(x, y, _("V:")));
	add_tool(automation[2] = new AutomatedFn(client, this, x + 80, y, 2));
	y += 20;
	add_tool(v_slider = new VSlider(client, x, y));
}

int YUVWindow::close_event()
{
	client->save_defaults();
	hide_window();
	client->send_hide_gui();
}

YSlider::YSlider(YUVMain *client, int x, int y)
 : BC_ISlider(x, y, 190, 30, 200, client->y, -MAXVALUE, MAXVALUE, DKGREY, BLACK, 1)
{
	this->client = client;
}
YSlider::~YSlider()
{
}
int YSlider::handle_event()
{
	client->y = get_value();
	client->send_configure_change();
}

USlider::USlider(YUVMain *client, int x, int y)
 : BC_ISlider(x, y, 190, 30, 200, client->u, -MAXVALUE, MAXVALUE, DKGREY, BLACK, 1)
{
	this->client = client;
}
USlider::~USlider()
{
}
int USlider::handle_event()
{
	client->u = get_value();
	client->send_configure_change();
}

VSlider::VSlider(YUVMain *client, int x, int y)
 : BC_ISlider(x, y, 190, 30, 200, client->v, -MAXVALUE, MAXVALUE, DKGREY, BLACK, 1)
{
	this->client = client;
}
VSlider::~VSlider()
{
}
int VSlider::handle_event()
{
	client->v = get_value();
	client->send_configure_change();
}

AutomatedFn::AutomatedFn(YUVMain *client, YUVWindow *window, int x, int y, int number)
 : BC_CheckBox(x, y, 16, 16, client->automated_function == number, _("Automate"))
{
	this->client = client;
	this->window = window;
	this->number = number;
}

AutomatedFn::~AutomatedFn()
{
}

int AutomatedFn::handle_event()
{
	for(int i = 0; i < 3; i++)
	{
		if(i != number) window->automation[i]->update(0);
	}
	update(1);
	client->automated_function = number;
	client->send_configure_change();
}


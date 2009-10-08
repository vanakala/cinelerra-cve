
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

#include "polarwindow.h"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)


PLUGIN_THREAD_OBJECT(PolarMain, PolarThread, PolarWindow)







PolarWindow::PolarWindow(PolarMain *client)
 : BC_Window("", 
 	MEGREY, 
	client->gui_string, 
	210, 
	120, 
	200, 
	120, 
	0, 
	!client->show_initially)
{
	this->client = client; 
}

PolarWindow::~PolarWindow()
{
	delete depth_slider;
	delete angle_slider;
	delete automation[0];
	delete automation[1];
}

int PolarWindow::create_objects()
{
	int x = 10, y = 10;
	add_tool(new BC_Title(x, y, _("Depth")));
	add_tool(automation[0] = new AutomatedFn(client, this, x + 80, y, 0));
	y += 20;
	add_tool(depth_slider = new DepthSlider(client, x, y));
	y += 35;
	add_tool(new BC_Title(x, y, _("Angle")));
	add_tool(automation[1] = new AutomatedFn(client, this, x + 80, y, 1));
	y += 20;
	add_tool(angle_slider = new AngleSlider(client, x, y));
}

int PolarWindow::close_event()
{
	client->save_defaults();
	hide_window();
	client->send_hide_gui();
}

DepthSlider::DepthSlider(PolarMain *client, int x, int y)
 : BC_ISlider(x, y, 190, 30, 200, client->depth, 0, MAXDEPTH, DKGREY, BLACK, 1)
{
	this->client = client;
}
DepthSlider::~DepthSlider()
{
}
int DepthSlider::handle_event()
{
	client->depth = get_value();
	client->send_configure_change();
}

AngleSlider::AngleSlider(PolarMain *client, int x, int y)
 : BC_ISlider(x, y, 190, 30, 200, client->angle, 0, MAXANGLE, DKGREY, BLACK, 1)
{
	this->client = client;
}
AngleSlider::~AngleSlider()
{
}
int AngleSlider::handle_event()
{
	client->angle = get_value();
	client->send_configure_change();
}

AutomatedFn::AutomatedFn(PolarMain *client, PolarWindow *window, int x, int y, int number)
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
	for(int i = 0; i < 2; i++)
	{
		if(i != number) window->automation[i]->update(0);
	}
	update(1);
	client->automated_function = number;
	client->send_configure_change();
}


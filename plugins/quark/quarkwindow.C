
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
#include "quarkwindow.h"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)


SharpenThread::SharpenThread(SharpenMain *client)
 : Thread()
{
	this->client = client;
	set_synchronous(0);
	gui_started.lock();
	completion.lock();
}

SharpenThread::~SharpenThread()
{
// Window always deleted here
	delete window;
}
	
void SharpenThread::run()
{
	BC_DisplayInfo info;
	window = new SharpenWindow(client, 
		info.get_abs_cursor_x() - 105, 
		info.get_abs_cursor_y() - 60);
	window->create_objects();
	gui_started.unlock();
	int result = window->run_window();
	completion.unlock();
// Last command executed in thread
	if(result) client->client_side_close();
}






SharpenWindow::SharpenWindow(SharpenMain *client, int x, int y)
 : BC_Window(client->gui_string, 
	x,
	y,
	210, 
	120, 
	210, 
	120, 
	0, 
	0,
	1)
{ 
	this->client = client; 
}

SharpenWindow::~SharpenWindow()
{
}

int SharpenWindow::create_objects()
{
	int x = 10, y = 10;
	add_tool(new BC_Title(x, y, _("Sharpness")));
	y += 20;
	add_tool(sharpen_slider = new SharpenSlider(client, &(client->sharpness), x, y));
	y += 30;
	add_tool(sharpen_interlace = new SharpenInterlace(client, x, y));
	y += 30;
	add_tool(sharpen_horizontal = new SharpenHorizontal(client, x, y));
	y += 30;
	add_tool(sharpen_luminance = new SharpenLuminance(client, x, y));
	show_window();
	flush();
	return 0;
}

int SharpenWindow::close_event()
{
// Set result to 1 to indicate a client side close
	set_done(1);
	return 1;
}

SharpenSlider::SharpenSlider(SharpenMain *client, float *output, int x, int y)
 : BC_ISlider(x, 
 	y, 
	0,
	200, 
	200,
	0, 
	MAXSHARPNESS, 
	(int)*output, 
	0, 
	0, 
	0)
{
	this->client = client;
	this->output = output;
}
SharpenSlider::~SharpenSlider()
{
}
int SharpenSlider::handle_event()
{
	*output = get_value();
	client->send_configure_change();
	return 1;
}




SharpenInterlace::SharpenInterlace(SharpenMain *client, int x, int y)
 : BC_CheckBox(x, y, client->interlace, _("Interlace"))
{
	this->client = client;
}
SharpenInterlace::~SharpenInterlace()
{
}
int SharpenInterlace::handle_event()
{
	client->interlace = get_value();
	client->send_configure_change();
	return 1;
}




SharpenHorizontal::SharpenHorizontal(SharpenMain *client, int x, int y)
 : BC_CheckBox(x, y, client->horizontal, _("Horizontal only"))
{
	this->client = client;
}
SharpenHorizontal::~SharpenHorizontal()
{
}
int SharpenHorizontal::handle_event()
{
	client->horizontal = get_value();
	client->send_configure_change();
	return 1;
}



SharpenLuminance::SharpenLuminance(SharpenMain *client, int x, int y)
 : BC_CheckBox(x, y, client->luminance, _("Luminance only"))
{
	this->client = client;
}
SharpenLuminance::~SharpenLuminance()
{
}
int SharpenLuminance::handle_event()
{
	client->luminance = get_value();
	client->send_configure_change();
	return 1;
}


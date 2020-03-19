
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

#include "bctitle.h"
#include "blur.h"
#include "blurwindow.h"

PLUGIN_THREAD_METHODS

BlurWindow::BlurWindow(BlurMain *plugin, int x, int y)
 : PluginWindow(plugin->gui_string,
	x,
	y,
	150, 
	270)
{ 
	x = 10; 
	y = 10;

	add_subwindow(new BC_Title(x, y, _(plugin->plugin_title())));
	y += 20;
	add_subwindow(horizontal = new BlurHorizontal(plugin, x, y));
	y += 30;
	add_subwindow(vertical = new BlurVertical(plugin, x, y));
	y += 35;
	add_subwindow(radius = new BlurRadius(plugin, x, y));
	add_subwindow(new BC_Title(x + 50, y, _("Radius")));
	y += 50;
	add_subwindow(a = new BlurA(plugin, x, y));
	y += 30;
	add_subwindow(r = new BlurR(plugin, x, y));
	y += 30;
	add_subwindow(g = new BlurG(plugin, x, y));
	y += 30;
	add_subwindow(b = new BlurB(plugin, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void BlurWindow::update()
{
	horizontal->update(plugin->config.horizontal);
	vertical->update(plugin->config.vertical);
	radius->update(plugin->config.radius);
	a->update(plugin->config.a);
	r->update(plugin->config.r);
	g->update(plugin->config.g);
	b->update(plugin->config.b);
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

int BlurRadius::handle_event()
{
	client->config.radius = get_value();
	client->send_configure_change();
	return 1;
}

BlurVertical::BlurVertical(BlurMain *client, int x, int y)
 : BC_CheckBox(x, 
	y, 
	client->config.vertical, 
	_("Vertical"))
{
	this->client = client;
}

int BlurVertical::handle_event()
{
	client->config.vertical = get_value();
	client->send_configure_change();
	return 1;
}

BlurHorizontal::BlurHorizontal(BlurMain *client, int x, int y)
 : BC_CheckBox(x, 
	y,
	client->config.horizontal, 
	_("Horizontal"))
{
	this->client = client;
}

int BlurHorizontal::handle_event()
{
	client->config.horizontal = get_value();
	client->send_configure_change();
	return 1;
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

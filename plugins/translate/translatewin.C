
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
#include "clip.h"
#include "translatewin.h"


PLUGIN_THREAD_METHODS


TranslateWin::TranslateWin(TranslateMain *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y,
	300, 
	220)
{ 
	x = y = 10;

	add_tool(new BC_Title(x, y, _("In X:")));
	y += 20;
	in_x = new TranslateCoord(this, plugin, x, y, &plugin->config.in_x);
	y += 30;

	add_tool(new BC_Title(x, y, _("In Y:")));
	y += 20;
	in_y = new TranslateCoord(this, plugin, x, y, &plugin->config.in_y);
	y += 30;

	add_tool(new BC_Title(x, y, _("In W:")));
	y += 20;
	in_w = new TranslateCoord(this, plugin, x, y, &plugin->config.in_w);
	y += 30;

	add_tool(new BC_Title(x, y, _("In H:")));
	y += 20;
	in_h = new TranslateCoord(this, plugin, x, y, &plugin->config.in_h);
	y += 30;

	x += 150;
	y = 10;
	add_tool(new BC_Title(x, y, _("Out X:")));
	y += 20;
	out_x = new TranslateCoord(this, plugin, x, y, &plugin->config.out_x);
	y += 30;

	add_tool(new BC_Title(x, y, _("Out Y:")));
	y += 20;
	out_y = new TranslateCoord(this, plugin, x, y, &plugin->config.out_y);
	y += 30;

	add_tool(new BC_Title(x, y, _("Out W:")));
	y += 20;
	out_w = new TranslateCoord(this, plugin, x, y, &plugin->config.out_w);
	y += 30;

	add_tool(new BC_Title(x, y, _("Out H:")));
	y += 20;
	out_h = new TranslateCoord(this, plugin, x, y, &plugin->config.out_h);
	y += 30;

	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

TranslateWin::~TranslateWin()
{
}

void TranslateWin::update()
{
	in_x->update(plugin->config.in_x);
	in_y->update(plugin->config.in_y);
	in_w->update(plugin->config.in_w);
	in_h->update(plugin->config.in_h);
	out_x->update(plugin->config.out_x);
	out_y->update(plugin->config.out_y);
	out_w->update(plugin->config.out_w);
	out_h->update(plugin->config.out_h);
}


TranslateCoord::TranslateCoord(TranslateWin *win, 
	TranslateMain *client, 
	int x, 
	int y,
	float *value)
 : BC_TumbleTextBox(win,
	(int)*value,
	(int)0,
	(int)1000,
	x, 
	y, 
	100)
{
	this->client = client;
	this->win = win;
	this->value = value;
}

TranslateCoord::~TranslateCoord()
{
}

int TranslateCoord::handle_event()
{
	*value = atof(get_text());

	client->send_configure_change();
	return 1;
}

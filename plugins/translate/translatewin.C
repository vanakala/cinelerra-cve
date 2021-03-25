// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "cinelerra.h"
#include "clip.h"
#include "translatewin.h"


PLUGIN_THREAD_METHODS


TranslateWin::TranslateWin(TranslateMain *plugin, int x, int y)
 : PluginWindow(plugin,
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
	double *value)
 : BC_TumbleTextBox(win,
	*value,
	(double)-MAX_FRAME_HEIGHT,
	(double)MAX_FRAME_HEIGHT,
	x, y, 100, 0)
{
	this->client = client;
	this->value = value;
}

int TranslateCoord::handle_event()
{
	*value = atof(get_text());

	client->send_configure_change();
	return 1;
}

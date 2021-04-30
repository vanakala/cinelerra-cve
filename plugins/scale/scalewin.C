// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "clip.h"
#include "language.h"
#include "scale.h"

PLUGIN_THREAD_METHODS

ScaleWin::ScaleWin(ScaleMain *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	240,
	150)
{ 
	x = y = 10;

	add_tool(new BC_Title(x, y, _("X Scale:")));
	y += 20;
	width = new ScaleWidth(this, plugin, x, y);
	y += 30;
	add_tool(new BC_Title(x, y, _("Y Scale:")));
	y += 20;
	height = new ScaleHeight(this, plugin, x, y);
	y += 35;
	add_tool(constrain = new ScaleConstrain(plugin, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void ScaleWin::update()
{
	width->update((float)plugin->config.w);
	height->update((float)plugin->config.h);
	constrain->update(plugin->config.constrain);
}


ScaleWidth::ScaleWidth(ScaleWin *win, 
	ScaleMain *client, 
	int x, 
	int y)
 : BC_TumbleTextBox(win,
	client->config.w,
	0.0,
	100.0,
	x, 
	y, 
	100)
{
	this->client = client;
	this->win = win;
	set_increment(0.1);
}

int ScaleWidth::handle_event()
{
	client->config.w = atof(get_text());
	CLAMP(client->config.w, 0, 100);

	if(client->config.constrain)
	{
		client->config.h = client->config.w;
		win->height->update((float)client->config.h);
	}

	client->send_configure_change();
	return 1;
}


ScaleHeight::ScaleHeight(ScaleWin *win, ScaleMain *client, int x, int y)
 : BC_TumbleTextBox(win,
	client->config.h,
	0.0,
	100.0,
	x, 
	y, 
	100)
{
	this->client = client;
	this->win = win;
	set_increment(0.1);
}

int ScaleHeight::handle_event()
{
	client->config.h = atof(get_text());
	CLAMP(client->config.h, 0, 100);

	if(client->config.constrain)
	{
		client->config.w = client->config.h;
		win->width->update((float)client->config.w);
	}

	client->send_configure_change();
	return 1;
}


ScaleConstrain::ScaleConstrain(ScaleMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.constrain, _("Constrain ratio"))
{
	this->client = client;
}

int ScaleConstrain::handle_event()
{
	client->config.constrain = get_value();
	client->send_configure_change();
	return 1;
}

// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "ivtcwindow.h"
#include "language.h"


static const char *pattern_text[] =
{
	N_("A  B  BC  CD  D"),
	N_("AB  BC  CD  DE  EF"),
	N_("Automatic")
};


PLUGIN_THREAD_METHODS


IVTCWindow::IVTCWindow(IVTCMain *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	210, 
	230)
{
	x = y = 10;

	add_tool(new BC_Title(x, y, _("Pattern offset:")));
	y += 20;
	add_tool(frame_offset = new IVTCOffset(plugin, x, y));
	y += 30;
	add_tool(first_field = new IVTCFieldOrder(plugin, x, y));
	y += 40;
	add_subwindow(new BC_Title(x, y, _("Pattern:")));
	y += 20;
	for(int i = 0; i < TOTAL_PATTERNS; i++)
	{
		add_subwindow(pattern[i] = new IVTCPattern(plugin,
			this, 
			i, 
			_(pattern_text[i]), 
			x, 
			y));
		y += 20;
	}
	
	if(plugin->config.pattern == IVTCConfig::AUTOMATIC)
	{
		frame_offset->disable();
		first_field->disable();
	}
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void IVTCWindow::update()
{
	if(plugin->config.pattern == IVTCConfig::AUTOMATIC)
	{
		frame_offset->disable();
		first_field->disable();
	}
	else
	{
		frame_offset->enable();
		first_field->enable();
	}
	frame_offset->update(plugin->config.frame_offset);
	first_field->update(plugin->config.first_field);

	for(int i = 0; i < TOTAL_PATTERNS; i++)
	{
		pattern[i]->update(plugin->config.pattern == i);
	}
}


IVTCOffset::IVTCOffset(IVTCMain *client, int x, int y)
 : BC_TextBox(x, 
	y,
	190,
	1, 
	client->config.frame_offset)
{
	this->client = client;
}

int IVTCOffset::handle_event()
{
	client->config.frame_offset = atol(get_text());
	client->send_configure_change();
	return 1;
}


IVTCFieldOrder::IVTCFieldOrder(IVTCMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.first_field, _("Odd field first"))
{
	this->client = client;
}

int IVTCFieldOrder::handle_event()
{
	client->config.first_field = get_value();
	client->send_configure_change();
	return 1;
}


IVTCPattern::IVTCPattern(IVTCMain *client, 
	IVTCWindow *window, 
	int number, 
	const char *text,
	int x, 
	int y)
 : BC_Radial(x, y, client->config.pattern == number, text)
{
	this->window = window;
	this->client = client;
	this->number = number;
}

int IVTCPattern::handle_event()
{
	client->config.pattern = number;
	if(number == IVTCConfig::AUTOMATIC)
	{
		window->frame_offset->disable();
		window->first_field->disable();
	}
	else
	{
		window->frame_offset->enable();
		window->first_field->enable();
	}

	for(int i = 0; i < TOTAL_PATTERNS; i++)
	{
		if(i != number) window->pattern[i]->update(0);
	}
	update(1);
	client->send_configure_change();
	return 1;
}

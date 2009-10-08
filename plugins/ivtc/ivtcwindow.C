
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
#include "ivtcwindow.h"
#include "language.h"


static char *pattern_text[] =
{
	"A  B  BC  CD  D",
	"AB  BC  CD  DE  EF",
	"Automatic",
	N_("A  B  BC  CD  D"),
	N_("AB  BC  CD  DE  EF"),
	N_("Automatic")
};


PLUGIN_THREAD_OBJECT(IVTCMain, IVTCThread, IVTCWindow)






IVTCWindow::IVTCWindow(IVTCMain *client, int x, int y)
 : BC_Window(client->gui_string, 
	x,
	y,
	210, 
	230, 
	210, 
	230, 
	0, 
	0,
	1)
{ 
	this->client = client; 
}

IVTCWindow::~IVTCWindow()
{
}

int IVTCWindow::create_objects()
{
	int x = 10, y = 10;
	
	add_tool(new BC_Title(x, y, _("Pattern offset:")));
	y += 20;
	add_tool(frame_offset = new IVTCOffset(client, x, y));
	y += 30;
	add_tool(first_field = new IVTCFieldOrder(client, x, y));
//	y += 30;
//	add_tool(automatic = new IVTCAuto(client, x, y));
	y += 40;
	add_subwindow(new BC_Title(x, y, _("Pattern:")));
	y += 20;
	for(int i = 0; i < TOTAL_PATTERNS; i++)
	{
		add_subwindow(pattern[i] = new IVTCPattern(client, 
			this, 
			i, 
			_(pattern_text[i]), 
			x, 
			y));
		y += 20;
	}
	
	if(client->config.pattern == IVTCConfig::AUTOMATIC)
	{
		frame_offset->disable();
		first_field->disable();
	}
//	y += 30;
//	add_tool(new BC_Title(x, y, _("Field threshold:")));
//	y += 20;
//	add_tool(threshold = new IVTCAutoThreshold(client, x, y));
	show_window();
	flush();
	return 0;
}

WINDOW_CLOSE_EVENT(IVTCWindow)

IVTCOffset::IVTCOffset(IVTCMain *client, int x, int y)
 : BC_TextBox(x, 
 	y, 
	190,
	1, 
	client->config.frame_offset)
{
	this->client = client;
}
IVTCOffset::~IVTCOffset()
{
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
IVTCFieldOrder::~IVTCFieldOrder()
{
}
int IVTCFieldOrder::handle_event()
{
	client->config.first_field = get_value();
	client->send_configure_change();
	return 1;
}


IVTCAuto::IVTCAuto(IVTCMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.automatic, _("Automatic IVTC"))
{
	this->client = client;
}
IVTCAuto::~IVTCAuto()
{
}
int IVTCAuto::handle_event()
{
	client->config.automatic = get_value();
	client->send_configure_change();
	return 1;
}

IVTCPattern::IVTCPattern(IVTCMain *client, 
	IVTCWindow *window, 
	int number, 
	char *text, 
	int x, 
	int y)
 : BC_Radial(x, y, client->config.pattern == number, text)
{
	this->window = window;
	this->client = client;
	this->number = number;
}
IVTCPattern::~IVTCPattern()
{
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



IVTCAutoThreshold::IVTCAutoThreshold(IVTCMain *client, int x, int y)
 : BC_TextBox(x, y, 190, 1, client->config.auto_threshold)
{
	this->client = client;
}
IVTCAutoThreshold::~IVTCAutoThreshold()
{
}
int IVTCAutoThreshold::handle_event()
{
	client->config.auto_threshold = atof(get_text());
	client->send_configure_change();
	return 1;
}


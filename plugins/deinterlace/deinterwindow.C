
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
#include "deinterwindow.h"
#include <string.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)



PLUGIN_THREAD_OBJECT(DeInterlaceMain, DeInterlaceThread, DeInterlaceWindow)




DeInterlaceWindow::DeInterlaceWindow(DeInterlaceMain *client, int x, int y)
 : BC_Window(client->gui_string, 
 	x, 
	y, 
	400, 
	200, 
	400, 
	200, 
	0, 
	0,
	1)
{ 
	this->client = client; 
	adaptive=0; dominance_top=0; dominance_bottom=0; threshold=0;

}

DeInterlaceWindow::~DeInterlaceWindow()
{
}

int DeInterlaceWindow::create_objects()
{
	int x = 10, y = 10;
	add_tool(new BC_Title(x, y, _("Select deinterlacing mode")));
	y += 25;
	add_tool(mode = new DeInterlaceMode(client, this, x, y));
	mode->create_objects();
	y += 25;
	optional_controls_x=x;
	optional_controls_y=y;
	y += 125;
	char string[BCTEXTLEN];
	get_status_string(string, 0);
	add_tool(status = new BC_Title(x, y, string));
	flash();
	show_window();
	set_mode(client->config.mode,0);
	return 0;
}

WINDOW_CLOSE_EVENT(DeInterlaceWindow)

void DeInterlaceWindow::get_status_string(char *string, int changed_rows)
{
	sprintf(string, _("Changed rows: %d\n"), changed_rows);
}

int DeInterlaceWindow::set_mode(int mode, int recursive)
{
	int x,y;
	client->config.mode = mode;
	
/* Restore position of controls */
	x=optional_controls_x;
	y=optional_controls_y;
	if (adaptive) { delete adaptive; adaptive=0; }
	if (threshold) { delete threshold; threshold=0; }
	if (dominance_top) { delete dominance_top; dominance_top=0; }
	if (dominance_bottom) { delete dominance_bottom; dominance_bottom=0; }

/* Display Dominance controls */
	switch (mode) 
	{
		case DEINTERLACE_KEEP:
		case DEINTERLACE_BOBWEAVE:
			add_subwindow(dominance_top = new DeInterlaceDominanceTop(client, this, x, y, _("Keep top field")));
			y+=25;
			add_subwindow(dominance_bottom = new DeInterlaceDominanceBottom(client, this, x, y, _("Keep bottom field")));
			y+=25;
			break;
		case DEINTERLACE_AVG_1F: 
			add_subwindow(dominance_top = new DeInterlaceDominanceTop(client, this, x, y, _("Average top fields")));
			y+=25;
			add_subwindow(dominance_bottom = new DeInterlaceDominanceBottom(client, this, x, y,"Average bottom fields"));
			y+=25;
			break;
		case DEINTERLACE_SWAP:
			add_subwindow(dominance_top = new DeInterlaceDominanceTop(client, this, x, y, _("Top field first")));
			y+=25;
			add_subwindow(dominance_bottom = new DeInterlaceDominanceBottom(client, this, x, y, _("Bottom field first")));
			y+=25;
			break;
		case DEINTERLACE_TEMPORALSWAP:
			add_subwindow(dominance_top = new DeInterlaceDominanceTop(client, this, x, y, _("Top field first")));
			y+=25;
			add_subwindow(dominance_bottom = new DeInterlaceDominanceBottom(client, this, x, y, _("Bottom field first")));
			y+=25;
			break;
		case DEINTERLACE_NONE:
		case  DEINTERLACE_AVG:
		default:
			;
	}

	if (dominance_top&&dominance_bottom)  {
		dominance_top->update(client->config.dominance?0:BC_Toggle::TOGGLE_CHECKED);
		dominance_bottom->update(client->config.dominance?BC_Toggle::TOGGLE_CHECKED:0);
	}
	
/* Display Threshold and adaptive controls */
	switch (mode) {
		case  DEINTERLACE_AVG_1F:
			add_subwindow(adaptive = new DeInterlaceAdaptive(client, x, y));

			add_subwindow(threshold = new DeInterlaceThreshold(client, x + 150, y));
			add_subwindow(threshold->title_caption=new BC_Title(x+150, y + 50, _("Threshold")));
			adaptive->update(client->config.adaptive?BC_Toggle::TOGGLE_CHECKED:0);
			break;
		case DEINTERLACE_BOBWEAVE:
			add_subwindow(threshold = new DeInterlaceThreshold(client, x + 150, y));
			add_subwindow(threshold->title_caption=new BC_Title(x+150, y + 50, _("Bob Threshold")));
			break;
		case DEINTERLACE_NONE:
		case DEINTERLACE_KEEP:
		case DEINTERLACE_AVG:
		case DEINTERLACE_SWAP:
		case DEINTERLACE_TEMPORALSWAP:
		default:

		break;
	}	


	if(!recursive)
		client->send_configure_change();
	return 0;
}


DeInterlaceOption::DeInterlaceOption(DeInterlaceMain *client, 
		DeInterlaceWindow *window, 
		int output, 
		int x, 
		int y, 
		char *text)
 : BC_Radial(x, y, client->config.mode == output, text)
{
	this->client = client;
	this->window = window;
	this->output = output;
}

DeInterlaceOption::~DeInterlaceOption()
{
}
int DeInterlaceOption::handle_event()
{
	window->set_mode(output, 0);
	return 1;
}


DeInterlaceAdaptive::DeInterlaceAdaptive(DeInterlaceMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.adaptive, _("Adaptive"))
{
	this->client = client;
}
int DeInterlaceAdaptive::handle_event()
{
	client->config.adaptive = get_value();
	client->send_configure_change();
	return 1;
}

DeInterlaceDominanceTop::DeInterlaceDominanceTop(DeInterlaceMain *client, DeInterlaceWindow *window, int x, int y, char * title)
 : BC_Radial(x, y, client->config.dominance, title)
{
	this->client = client;
	this->window = window;

}
int DeInterlaceDominanceTop::handle_event()
{
	client->config.dominance = (get_value()==0);
	window->dominance_bottom->update(client->config.dominance?BC_Toggle::TOGGLE_CHECKED:0);
	client->send_configure_change();
	return 1;
}


DeInterlaceDominanceBottom::DeInterlaceDominanceBottom(DeInterlaceMain *client, DeInterlaceWindow *window, int x, int y, char * title)
 : BC_Radial(x, y, client->config.dominance, title)
{
	this->client = client;
	this->window = window;
}
int DeInterlaceDominanceBottom::handle_event()
{

	client->config.dominance = (get_value() != 0 );
	window->dominance_top->update(client->config.dominance?0:BC_Toggle::TOGGLE_CHECKED);
	client->send_configure_change();
	return 1;
}


DeInterlaceThreshold::DeInterlaceThreshold(DeInterlaceMain *client, int x, int y)
 : BC_IPot(x, y, client->config.threshold, 0, 100)
{
	this->client = client;
	title_caption=NULL;
}
int DeInterlaceThreshold::handle_event()
{
	client->config.threshold = get_value();
	client->send_configure_change();
	return 1;
}

DeInterlaceThreshold::~DeInterlaceThreshold()
{
  if (title_caption) delete title_caption;
}

DeInterlaceMode::DeInterlaceMode(DeInterlaceMain*plugin, 
	DeInterlaceWindow *gui, 
	int x, 
	int y)
 : BC_PopupMenu(x, y, 200, to_text(plugin->config.mode), 1)
{
	this->plugin = plugin;
	this->gui = gui;
}
void DeInterlaceMode::create_objects()
{
	add_item(new BC_MenuItem(to_text(DEINTERLACE_NONE)));
	add_item(new BC_MenuItem(to_text(DEINTERLACE_KEEP)));
	add_item(new BC_MenuItem(to_text(DEINTERLACE_AVG)));
	add_item(new BC_MenuItem(to_text(DEINTERLACE_AVG_1F)));
	add_item(new BC_MenuItem(to_text(DEINTERLACE_BOBWEAVE)));
	add_item(new BC_MenuItem(to_text(DEINTERLACE_SWAP)));
	add_item(new BC_MenuItem(to_text(DEINTERLACE_TEMPORALSWAP)));
}

char* DeInterlaceMode::to_text(int mode)
{
	switch(mode)
	{
		case DEINTERLACE_KEEP:
			return _("Duplicate one field");
		case DEINTERLACE_AVG_1F:
			return _("Average one field");
		case DEINTERLACE_AVG:
			return _("Average both fields");
		case DEINTERLACE_BOBWEAVE:
			return _("Bob & Weave");
		case DEINTERLACE_SWAP:
			return _("Spatial field swap");
		case DEINTERLACE_TEMPORALSWAP:
			return _("Temporal field swap");
		default:
			return _("Do Nothing");
	}
}
int DeInterlaceMode::from_text(char *text)
{
	if(!strcmp(text, to_text(DEINTERLACE_KEEP))) 
		return DEINTERLACE_KEEP;
	if(!strcmp(text, to_text(DEINTERLACE_AVG))) 
		return DEINTERLACE_AVG;
	if(!strcmp(text, to_text(DEINTERLACE_AVG_1F))) 
		return DEINTERLACE_AVG_1F;
	if(!strcmp(text, to_text(DEINTERLACE_BOBWEAVE))) 
		return DEINTERLACE_BOBWEAVE;
	if(!strcmp(text, to_text(DEINTERLACE_SWAP))) 
		return DEINTERLACE_SWAP;
	if(!strcmp(text, to_text(DEINTERLACE_TEMPORALSWAP))) 
		return DEINTERLACE_TEMPORALSWAP;
	return DEINTERLACE_NONE;
}

int DeInterlaceMode::handle_event()
{
	plugin->config.mode = from_text(get_text());
	gui->set_mode(plugin->config.mode,0);
	plugin->send_configure_change();
}




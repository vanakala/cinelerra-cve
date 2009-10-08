
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
#include "agingwindow.h"


#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

PLUGIN_THREAD_OBJECT(AgingMain, AgingThread, AgingWindow)





AgingWindow::AgingWindow(AgingMain *client, int x, int y)
 : BC_Window(client->gui_string, 
	x,
	y,
	300, 
	170, 
	300, 
	170, 
	0, 
	0,
	1)
{ 
	this->client = client; 
}

AgingWindow::~AgingWindow()
{
}

int AgingWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, 
		_("Film aging from EffectTV\n"
		"Copyright (C) 2001 FUKUCHI Kentarou")
	));
// 	
// 	y += 50;
// 	add_subwindow(color = new AgingColor(x, y, client));	
// 	
// 	y += 25;
// 	add_subwindow(scratches = new AgingScratches(x, y, client));
// 	add_subwindow(scratch_count = new AgingScratchCount(x + 100, y + 10, client));
// 	
// 	y += 25;
// 	add_subwindow(pits = new AgingPits(x, y, client));
// 	add_subwindow(pit_count = new AgingPitCount(x + 100, y + 10, client));
// 	
// 	y += 25;
// 	add_subwindow(dust = new AgingDust(x, y, client));
// 	add_subwindow(dust_count = new AgingDustCount(x + 100, y + 10, client));

	show_window();
	flush();
	return 0;
}

WINDOW_CLOSE_EVENT(AgingWindow)







AgingColor::AgingColor(int x, int y, AgingMain *plugin)
 : BC_CheckBox(x, y, plugin->config.colorage, _("Grain"))
{
	this->plugin = plugin;
}

int AgingColor::handle_event()
{
	return 1;
}





AgingScratches::AgingScratches(int x, int y, AgingMain *plugin)
 : BC_CheckBox(x, y, plugin->config.scratch, _("Scratch"))
{
	this->plugin;
}

int AgingScratches::handle_event()
{
	return 1;
}









AgingScratchCount::AgingScratchCount(int x, int y, AgingMain *plugin)
 : BC_ISlider(x, 
			y,
			0,
			180, 
			180, 
			0, 
			SCRATCH_MAX, 
			plugin->config.scratch_lines)
{
	this->plugin = plugin;
}

int AgingScratchCount::handle_event()
{
	return 1;
}






AgingPits::AgingPits(int x, int y, AgingMain *plugin)
 : BC_CheckBox(x, y, plugin->config.pits, _("Pits"))
{
	this->plugin = plugin;
}

int AgingPits::handle_event()
{
	return 1;
}






AgingPitCount::AgingPitCount(int x, int y, AgingMain *plugin)
 : BC_ISlider(x, 
			y,
			0,
			180, 
			180, 
			0, 
			100, 
			plugin->config.pit_count)
{
	this->plugin = plugin;
}

int AgingPitCount::handle_event()
{
	return 1;
}









AgingDust::AgingDust(int x, int y, AgingMain *plugin)
 : BC_CheckBox(x, y, plugin->config.dust, _("Dust"))
{
	this->plugin = plugin;
}

int AgingDust::handle_event()
{
	return 1;
}





AgingDustCount::AgingDustCount(int x, int y, AgingMain *plugin)
 : BC_ISlider(x, 
			y,
			0,
			180, 
			180, 
			0, 
			100, 
			plugin->config.dust_count)
{
	this->plugin = plugin;
}

int AgingDustCount::handle_event()
{
	return 1;
}







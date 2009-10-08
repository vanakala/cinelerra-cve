
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
#include "despikewindow.h"

#include <string.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)


PLUGIN_THREAD_OBJECT(Despike, DespikeThread, DespikeWindow)






DespikeWindow::DespikeWindow(Despike *despike, int x, int y)
 : BC_Window(despike->gui_string, 
 	x, 
	y, 
	230, 
	110, 
	230, 
	110, 
	0, 
	0,
	1)
{ 
	this->despike = despike; 
}

DespikeWindow::~DespikeWindow()
{
}

int DespikeWindow::create_objects()
{
	int x = 10, y = 10;
	add_tool(new BC_Title(5, y, _("Maximum level:")));
	y += 20;
	add_tool(level = new DespikeLevel(despike, x, y));
	y += 30;
	add_tool(new BC_Title(5, y, _("Maximum rate of change:")));
	y += 20;
	add_tool(slope = new DespikeSlope(despike, x, y));
	show_window();
	flush();
	return 0;
}

int DespikeWindow::close_event()
{
// Set result to 1 to indicate a client side close
	set_done(1);
	return 1;
}





DespikeLevel::DespikeLevel(Despike *despike, int x, int y)
 : BC_FSlider(x, 
 	y, 
	0,
	200,
	200,
	INFINITYGAIN, 
	0,
	despike->config.level)
{
	this->despike = despike;
}
int DespikeLevel::handle_event()
{
	despike->config.level = get_value();
	despike->send_configure_change();
	return 1;
}

DespikeSlope::DespikeSlope(Despike *despike, int x, int y)
 : BC_FSlider(x, 
 	y, 
	0,
	200,
	200,
	INFINITYGAIN, 
	0,
	despike->config.slope)
{
	this->despike = despike;
}
int DespikeSlope::handle_event()
{
	despike->config.slope = get_value();
	despike->send_configure_change();
	return 1;
}

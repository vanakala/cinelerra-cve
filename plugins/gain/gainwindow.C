
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
#include "bchash.h"
#include "filesystem.h"
#include "gainwindow.h"
#include "language.h"

#include <string.h>


PLUGIN_THREAD_OBJECT(Gain, GainThread, GainWindow)






GainWindow::GainWindow(Gain *gain, int x, int y)
 : BC_Window(gain->gui_string, 
 	x, 
	y, 
	230, 
	60, 
	230, 
	60, 
	0, 
	0,
	1)
{
	this->gain = gain;
}

GainWindow::~GainWindow()
{
}

int GainWindow::create_objects()
{
	int x = 10, y = 10;
	add_tool(new BC_Title(5, y, _("Level:")));
	y += 20;
	add_tool(level = new GainLevel(gain, x, y));
	show_window();
	flush();
	return 0;
}

WINDOW_CLOSE_EVENT(GainWindow)




GainLevel::GainLevel(Gain *gain, int x, int y)
 : BC_FSlider(x, 
 	y, 
	0,
	200,
	200,
	INFINITYGAIN, 
	40,
	gain->config.level)
{
	this->gain = gain;
}
int GainLevel::handle_event()
{
	gain->config.level = get_value();
	gain->send_configure_change();
	return 1;
}

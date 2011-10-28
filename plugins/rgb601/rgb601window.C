
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
#include "language.h"
#include "rgb601window.h"


PLUGIN_THREAD_METHODS


RGB601Window::RGB601Window(RGB601Main *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
	x,
	y,
	210, 
	200, 
	210, 
	200, 
	0, 
	0,
	1)
{
	x = y = 10;

	add_tool(forward = new RGB601Direction(this, 
		x, 
		y, 
		&plugin->config.direction, 
		1, 
		_("RGB -> 601 compression")));
	y += 30;
	add_tool(reverse = new RGB601Direction(this, 
		x, 
		y, 
		&plugin->config.direction, 
		2, 
		_("601 -> RGB expansion")));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

RGB601Window::~RGB601Window()
{
}

void RGB601Window::update()
{
	forward->update(plugin->config.direction == 1);
	reverse->update(plugin->config.direction == 2);
}


RGB601Direction::RGB601Direction(RGB601Window *window, int x, int y, int *output, int true_value, const char *text)
 : BC_CheckBox(x, y, *output == true_value, text)
{
	this->output = output;
	this->true_value = true_value;
	this->window = window;
}

RGB601Direction::~RGB601Direction()
{
}

int RGB601Direction::handle_event()
{
	*output = get_value() ? true_value : 0;
	window->update();
	window->plugin->send_configure_change();
	return 1;
}

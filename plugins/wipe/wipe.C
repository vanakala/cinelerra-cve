
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

#include "bchash.h"
#include "bctitle.h"
#include "filexml.h"
#include "picon_png.h"
#include "vframe.h"
#include "wipe.h"

#include <stdint.h>
#include <string.h>

REGISTER_PLUGIN

WipeLeft::WipeLeft(WipeMain *plugin, 
	WipeWindow *window,
	int x,
	int y)
 : BC_Radial(x, 
		y, 
		plugin->direction == 0, 
		_("Left"))
{
	this->plugin = plugin;
	this->window = window;
}

int WipeLeft::handle_event()
{
	update(1);
	plugin->direction = 0;
	window->right->update(0);
	plugin->send_configure_change();
	return 0;
}

WipeRight::WipeRight(WipeMain *plugin, 
	WipeWindow *window,
	int x,
	int y)
 : BC_Radial(x, 
		y, 
		plugin->direction == 1, 
		_("Right"))
{
	this->plugin = plugin;
	this->window = window;
}

int WipeRight::handle_event()
{
	update(1);
	plugin->direction = 1;
	window->left->update(0);
	plugin->send_configure_change();
	return 0;
}

WipeWindow::WipeWindow(WipeMain *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y, 
	320, 
	50)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Direction:")));
	x += 100;
	add_subwindow(left = new WipeLeft(plugin, 
		this,
		x,
		y));
	x += 100;
	add_subwindow(right = new WipeRight(plugin, 
		this,
		x,
		y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

PLUGIN_THREAD_METHODS

WipeMain::WipeMain(PluginServer *server)
 : PluginVClient(server)
{
	direction = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

WipeMain::~WipeMain()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void WipeMain::load_defaults()
{
	defaults = load_defaults_file("wipe.rc");

	direction = defaults->get("DIRECTION", direction);
}

void WipeMain::save_defaults()
{
	defaults->update("DIRECTION", direction);
	defaults->save();
}

void WipeMain::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("WIPE");
	output.tag.set_property("DIRECTION", direction);
	output.append_tag();
	output.tag.set_title("/WIPE");
	output.append_tag();
	output.terminate_string();
}

void WipeMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	while(!input.read_tag())
	{
		if(input.tag.title_is("WIPE"))
		{
			direction = input.tag.get_property("DIRECTION", direction);
		}
	}
}

int WipeMain::load_configuration()
{
	read_data(prev_keyframe_pts(source_pts));
	return 0;
}

#define WIPE(type, components) \
{ \
	if(direction == 0) \
	{ \
		for(int j = 0; j < h; j++) \
		{ \
			type *in_row = (type*)incoming->get_row_ptr(j); \
			type *out_row = (type*)outgoing->get_row_ptr(j); \
			int x = round(w * source_pts /  total_len_pts); \
 \
			for(int k = 0; k < x; k++) \
			{ \
				out_row[k * components + 0] = in_row[k * components + 0]; \
				out_row[k * components + 1] = in_row[k * components + 1]; \
				out_row[k * components + 2] = in_row[k * components + 2]; \
				if(components == 4) out_row[k * components + 3] = in_row[k * components + 3]; \
			} \
		} \
	} \
	else \
	{ \
		for(int j = 0; j < h; j++) \
		{ \
			type *in_row = (type*)incoming->get_row_ptr(j); \
			type *out_row = (type*)outgoing->get_row_ptr(j); \
			int x = w - (int)round(w * source_pts /  total_len_pts); \
 \
			for(int k = x; k < w; k++) \
			{ \
				out_row[k * components + 0] = in_row[k * components + 0]; \
				out_row[k * components + 1] = in_row[k * components + 1]; \
				out_row[k * components + 2] = in_row[k * components + 2]; \
				if(components == 4) out_row[k * components + 3] = in_row[k * components + 3]; \
			} \
		} \
	} \
}

void WipeMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	load_configuration();

	int w = incoming->get_w();
	int h = incoming->get_h();

	switch(incoming->get_color_model())
	{
		case BC_RGB_FLOAT:
			WIPE(float, 3)
			break;
		case BC_RGB888:
		case BC_YUV888:
			WIPE(unsigned char, 3)
			break;
		case BC_RGBA_FLOAT:
			WIPE(float, 4)
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			WIPE(unsigned char, 4)
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			WIPE(uint16_t, 3)
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			WIPE(uint16_t, 4)
			break;
	}
}

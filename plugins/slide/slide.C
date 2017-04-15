
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
#include "slide.h"


#include <stdint.h>
#include <string.h>

REGISTER_PLUGIN

SlideLeft::SlideLeft(SlideMain *plugin, 
	SlideWindow *window,
	int x,
	int y)
 : BC_Radial(x, 
		y, 
		plugin->motion_direction == 0, 
		_("Left"))
{
	this->plugin = plugin;
	this->window = window;
}

int SlideLeft::handle_event()
{
	update(1);
	plugin->motion_direction = 0;
	window->right->update(0);
	plugin->send_configure_change();
	return 0;
}

SlideRight::SlideRight(SlideMain *plugin, 
	SlideWindow *window,
	int x,
	int y)
 : BC_Radial(x, 
		y, 
		plugin->motion_direction == 1, 
		_("Right"))
{
	this->plugin = plugin;
	this->window = window;
}

int SlideRight::handle_event()
{
	update(1);
	plugin->motion_direction = 1;
	window->left->update(0);
	plugin->send_configure_change();
	return 0;
}

SlideIn::SlideIn(SlideMain *plugin, 
	SlideWindow *window,
	int x,
	int y)
 : BC_Radial(x, 
		y, 
		plugin->direction == 0, 
		_("In"))
{
	this->plugin = plugin;
	this->window = window;
}

int SlideIn::handle_event()
{
	update(1);
	plugin->direction = 0;
	window->out->update(0);
	plugin->send_configure_change();
	return 0;
}

SlideOut::SlideOut(SlideMain *plugin, 
	SlideWindow *window,
	int x,
	int y)
 : BC_Radial(x, 
		y, 
		plugin->direction == 1, 
		_("Out"))
{
	this->plugin = plugin;
	this->window = window;
}

int SlideOut::handle_event()
{
	update(1);
	plugin->direction = 1;
	window->in->update(0);
	plugin->send_configure_change();
	return 0;
}


SlideWindow::SlideWindow(SlideMain *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x, 
	y, 
	320, 
	100)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Direction:")));
	x += 100;
	add_subwindow(left = new SlideLeft(plugin, 
		this,
		x,
		y));
	x += 100;
	add_subwindow(right = new SlideRight(plugin, 
		this,
		x,
		y));

	y += 30;
	x = 10;
	add_subwindow(new BC_Title(x, y, _("Direction:")));
	x += 100;
	add_subwindow(in = new SlideIn(plugin, 
		this,
		x,
		y));
	x += 100;
	add_subwindow(out = new SlideOut(plugin, 
		this,
		x,
		y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

PLUGIN_THREAD_METHODS


SlideMain::SlideMain(PluginServer *server)
 : PluginVClient(server)
{
	motion_direction = 0;
	direction = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

SlideMain::~SlideMain()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void SlideMain::load_defaults()
{
	defaults = defaults = load_defaults_file("slide.rc");

	motion_direction = defaults->get("MOTION_DIRECTION", motion_direction);
	direction = defaults->get("DIRECTION", direction);
}

void SlideMain::save_defaults()
{
	defaults->update("MOTION_DIRECTION", motion_direction);
	defaults->update("DIRECTION", direction);
	defaults->save();
}

void SlideMain::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("SLIDE");
	output.tag.set_property("MOTION_DIRECTION", motion_direction);
	output.tag.set_property("DIRECTION", direction);
	output.append_tag();
	output.tag.set_title("/SLIDE");
	output.append_tag();
	output.terminate_string();
}

void SlideMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	while(!input.read_tag())
	{
		if(input.tag.title_is("SLIDE"))
		{
			motion_direction = input.tag.get_property("MOTION_DIRECTION", motion_direction);
			direction = input.tag.get_property("DIRECTION", direction);
		}
	}
}

int SlideMain::load_configuration()
{
	read_data(prev_keyframe_pts(source_pts));
	return 0;
}

#define SLIDE(type, components) \
{ \
	if(direction == 0) \
	{ \
		int in_add, out_add, cpy_len; \
		if(motion_direction == 0) \
		{ \
			int x = round(w * source_pts / total_len_pts); \
			out_add = 0; \
			in_add = (w - x) * components * sizeof(type); \
			cpy_len = x * components * sizeof(type); \
		} \
		else \
		{ \
			int x = w - (int)(round(w * source_pts / total_len_pts)); \
			out_add = x * components * sizeof(type); \
			in_add = 0; \
			cpy_len = (w - x) * components * sizeof(type); \
		} \
 \
		for(int j = 0; j < h; j++) \
		{ \
			memcpy( ((char *)outgoing->get_rows()[j]) + out_add, \
				((char *)incoming->get_rows()[j]) + in_add, \
				cpy_len); \
		} \
	} \
	else \
	{ \
		if(motion_direction == 0) \
		{ \
			int x = w - (int)(round(w * source_pts / total_len_pts)); \
			for(int j = 0; j < h; j++) \
			{ \
				char *in_row = (char*)incoming->get_rows()[j]; \
				char *out_row = (char*)outgoing->get_rows()[j]; \
				memmove(out_row + 0, out_row + ((w - x) * components * sizeof(type)), x * components * sizeof(type)); \
				memcpy (out_row + x * components * sizeof(type), in_row + x * components * sizeof (type), (w - x) * components * sizeof(type)); \
			} \
		} \
		else \
		{ \
			int x = round(w * source_pts / total_len_pts); \
			for(int j = 0; j < h; j++) \
			{ \
				char *in_row = (char*)incoming->get_rows()[j]; \
				char *out_row = (char*)outgoing->get_rows()[j]; \
 \
				memmove(out_row + (x * components *sizeof(type)), out_row + 0, (w - x) * components * sizeof(type)); \
				memcpy (out_row + 0, in_row + 0, (x) * components * sizeof(type)); \
			} \
		} \
	} \
}


void SlideMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	load_configuration();

	int w = incoming->get_w();
	int h = incoming->get_h();

	switch(incoming->get_color_model())
	{
	case BC_RGB_FLOAT:
		SLIDE(float, 3)
		break;
	case BC_RGBA_FLOAT:
		SLIDE(float, 4)
		break;
	case BC_RGB888:
	case BC_YUV888:
		SLIDE(unsigned char, 3)
		break;
	case BC_RGBA8888:
	case BC_YUVA8888:
		SLIDE(unsigned char, 4)
		break;
	case BC_RGB161616:
	case BC_YUV161616:
		SLIDE(uint16_t, 3)
		break;
	case BC_RGBA16161616:
	case BC_YUVA16161616:
		SLIDE(uint16_t, 4)
		break;
	}
}

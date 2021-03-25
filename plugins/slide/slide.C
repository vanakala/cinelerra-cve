// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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
 : BC_Radial(x, y, plugin->motion_direction == 0, _("Left"))
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
	return 1;
}

SlideRight::SlideRight(SlideMain *plugin, 
	SlideWindow *window,
	int x,
	int y)
 : BC_Radial(x, y, plugin->motion_direction == 1, _("Right"))
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
	return 1;
}

SlideIn::SlideIn(SlideMain *plugin, 
	SlideWindow *window,
	int x,
	int y)
 : BC_Radial(x, y, plugin->direction == 0, _("In"))
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
	return 1;
}

SlideOut::SlideOut(SlideMain *plugin, 
	SlideWindow *window,
	int x,
	int y)
 : BC_Radial(x, y, plugin->direction == 1, _("Out"))
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
	return 1;
}


SlideWindow::SlideWindow(SlideMain *plugin, int x, int y)
 : PluginWindow(plugin,
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

	output.tag.set_title("SLIDE");
	output.tag.set_property("MOTION_DIRECTION", motion_direction);
	output.tag.set_property("DIRECTION", direction);
	output.append_tag();
	output.tag.set_title("/SLIDE");
	output.append_tag();
	keyframe->set_data(output.string);
}

void SlideMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	if(!keyframe)
		return;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

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
	read_data(get_prev_keyframe(source_pts));
	return 0;
}

void SlideMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	int w = incoming->get_w();
	int h = incoming->get_h();
	int cmodel = incoming->get_color_model();

	load_configuration();

	switch(cmodel)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		if(direction == 0)
		{
			int in_add, out_add, cpy_len;

			if(motion_direction == 0)
			{
				int x = round(w * source_pts / get_length());
				out_add = 0;
				in_add = (w - x) * 4 * sizeof(uint16_t);
				cpy_len = x * 4 * sizeof(uint16_t);
			}
			else
			{
				int x = w - (int)(round(w * source_pts / get_length()));
				out_add = x * 4 * sizeof(uint16_t);
				in_add = 0;
				cpy_len = (w - x) * 4 * sizeof(uint16_t);
			}

			for(int j = 0; j < h; j++)
			{
				memcpy(((char*)outgoing->get_row_ptr(j)) + out_add,
					((char*)incoming->get_row_ptr(j)) + in_add,
					cpy_len);
			}
		}
		else
		{
			if(motion_direction == 0)
			{
				int x = w - (int)(round(w * source_pts / get_length()));

				for(int j = 0; j < h; j++)
				{
					char *in_row = (char*)incoming->get_row_ptr(j);
					char *out_row = (char*)outgoing->get_row_ptr(j);
					memmove(out_row,
						out_row + ((w - x) * 4 * sizeof(uint16_t)),
						x * 4 * sizeof(uint16_t));
					memcpy(out_row + x * 4 * sizeof(uint16_t),
						in_row + x * 4 * sizeof(uint16_t),
						(w - x) * 4 * sizeof(uint16_t));
				}
			}
			else
			{
				int x = round(w * source_pts / get_length());

				for(int j = 0; j < h; j++)
				{
					char *in_row = (char*)incoming->get_row_ptr(j);
					char *out_row = (char*)outgoing->get_row_ptr(j);

					memmove(out_row + (x * 4 *sizeof(uint16_t)),
						out_row, (w - x) * 4 * sizeof(uint16_t));
					memcpy(out_row, in_row,
						x * 4 * sizeof(uint16_t));
				}
			}
		}
		break;
	default:
		unsupported(cmodel);
		break;
	}
}

// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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
 : BC_Radial(x, y, plugin->direction == 0, _("Left"))
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
	return 1;
}

WipeRight::WipeRight(WipeMain *plugin, 
	WipeWindow *window,
	int x,
	int y)
 : BC_Radial(x, y, plugin->direction == 1, _("Right"))
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
	return 1;
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

	output.tag.set_title("WIPE");
	output.tag.set_property("DIRECTION", direction);
	output.append_tag();
	output.tag.set_title("/WIPE");
	output.append_tag();
	keyframe->set_data(output.string);
}

void WipeMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	if(!keyframe)
		return;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

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
	read_data(get_prev_keyframe(source_pts));
	return 0;
}


void WipeMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	int w = incoming->get_w();
	int h = incoming->get_h();
	ptstime total_len_pts = get_length();
	int cmodel = incoming->get_color_model();

	if(total_len_pts < EPSILON)
		return;

	load_configuration();

	switch(incoming->get_color_model())
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		if(direction == 0)
		{
			for(int j = 0; j < h; j++)
			{
				uint16_t *in_row = (uint16_t*)incoming->get_row_ptr(j);
				uint16_t *out_row = (uint16_t*)outgoing->get_row_ptr(j);

				int x = round(w * source_pts / total_len_pts);

				for(int k = 0; k < x; k++)
				{
					out_row[k * 4 + 0] = in_row[k * 4 + 0];
					out_row[k * 4 + 1] = in_row[k * 4 + 1];
					out_row[k * 4 + 2] = in_row[k * 4 + 2];
					out_row[k * 4 + 3] = in_row[k * 4 + 3];
				}
			}
		}
		else
		{
			for(int j = 0; j < h; j++)
			{
				uint16_t *in_row = (uint16_t*)incoming->get_row_ptr(j);
				uint16_t *out_row = (uint16_t*)outgoing->get_row_ptr(j);
				int x = w - (int)round(w * source_pts /  total_len_pts);

				for(int k = x; k < w; k++)
				{
					out_row[k * 4 + 0] = in_row[k * 4 + 0];
					out_row[k * 4 + 1] = in_row[k * 4 + 1];
					out_row[k * 4 + 2] = in_row[k * 4 + 2];
					out_row[k * 4 + 3] = in_row[k * 4 + 3];
				}
			}
		}
		break;
	default:
		unsupported(cmodel);
		break;
	}
}

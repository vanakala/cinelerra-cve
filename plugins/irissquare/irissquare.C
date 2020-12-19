// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>


#include "bchash.h"
#include "bctitle.h"
#include "filexml.h"
#include "picon_png.h"
#include "vframe.h"
#include "irissquare.h"

#include <stdint.h>
#include <string.h>

REGISTER_PLUGIN

IrisSquareIn::IrisSquareIn(IrisSquareMain *plugin, 
	IrisSquareWindow *window,
	int x,
	int y)
 : BC_Radial(x, y, plugin->direction == 0, _("In"))
{
	this->plugin = plugin;
	this->window = window;
}

int IrisSquareIn::handle_event()
{
	update(1);
	plugin->direction = 0;
	window->out->update(0);
	plugin->send_configure_change();
	return 1;
}

IrisSquareOut::IrisSquareOut(IrisSquareMain *plugin, 
	IrisSquareWindow *window,
	int x,
	int y)
 : BC_Radial(x, y, plugin->direction == 1, _("Out"))
{
	this->plugin = plugin;
	this->window = window;
}

int IrisSquareOut::handle_event()
{
	update(1);
	plugin->direction = 1;
	window->in->update(0);
	plugin->send_configure_change();
	return 1;
}


IrisSquareWindow::IrisSquareWindow(IrisSquareMain *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y, 
	320, 
	50)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Direction:")));
	x += 100;
	add_subwindow(in = new IrisSquareIn(plugin, 
		this,
		x,
		y));
	x += 100;
	add_subwindow(out = new IrisSquareOut(plugin, 
		this,
		x,
		y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

PLUGIN_THREAD_METHODS

IrisSquareMain::IrisSquareMain(PluginServer *server)
 : PluginVClient(server)
{
	direction = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

IrisSquareMain::~IrisSquareMain()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void IrisSquareMain::load_defaults()
{
	defaults = load_defaults_file("irissquare.rc");

	direction = defaults->get("DIRECTION", direction);
}

void IrisSquareMain::save_defaults()
{
	defaults->update("DIRECTION", direction);
	defaults->save();
}

void IrisSquareMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("IRISSQUARE");
	output.tag.set_property("DIRECTION", direction);
	output.append_tag();
	output.tag.set_title("/IRISSQUARE");
	output.append_tag();
	keyframe->set_data(output.string);
}

void IrisSquareMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	if(!keyframe)
		return;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("IRISSQUARE"))
		{
			direction = input.tag.get_property("DIRECTION", direction);
		}
	}
}

int IrisSquareMain::load_configuration()
{
	read_data(get_prev_keyframe(source_pts));
	return 0;
}

void IrisSquareMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	ptstime length = get_length();
	int cmodel = incoming->get_color_model();

	load_configuration();

	if(length < EPSILON)
		return;

	int w = incoming->get_w();
	int h = incoming->get_h();
	int dw = round(w / 2 * source_pts / length);
	int dh = round(h / 2 * source_pts / length);

	switch(cmodel)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		if(direction == 0)
		{
			int x1 = w / 2 - dw;
			int x2 = w / 2 + dw;
			int y1 = h / 2 - dh;
			int y2 = h / 2 + dh;

			for(int j = y1; j < y2; j++)
			{
				uint16_t *in_row = (uint16_t*)incoming->get_row_ptr(j);
				uint16_t *out_row = (uint16_t*)outgoing->get_row_ptr(j);
 
				for(int k = x1; k < x2; k++)
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
			int x1 = dw;
			int x2 = w - dw;
			int y1 = dh;
			int y2 = h - dh;

			for(int j = 0; j < y1; j++)
			{
				uint16_t *in_row = (uint16_t*)incoming->get_row_ptr(j);
				uint16_t *out_row = (uint16_t*)outgoing->get_row_ptr(j);

				for(int k = 0; k < w; k++)
				{
					out_row[k * 4 + 0] = in_row[k * 4 + 0];
					out_row[k * 4 + 1] = in_row[k * 4 + 1];
					out_row[k * 4 + 2] = in_row[k * 4 + 2];
					out_row[k * 4 + 3] = in_row[k * 4 + 3];
				}
			}
			for(int j = y1; j < y2; j++)
			{
				uint16_t *in_row = (uint16_t*)incoming->get_row_ptr(j);
				uint16_t *out_row = (uint16_t*)outgoing->get_row_ptr(j);

				for(int k = 0; k < x1; k++)
				{
					out_row[k * 4 + 0] = in_row[k * 4 + 0];
					out_row[k * 4 + 1] = in_row[k * 4 + 1];
					out_row[k * 4 + 2] = in_row[k * 4 + 2];
					out_row[k * 4 + 3] = in_row[k * 4 + 3];
				}

				for(int k = x2; k < w; k++)
				{
					out_row[k * 4 + 0] = in_row[k * 4 + 0];
					out_row[k * 4 + 1] = in_row[k * 4 + 1];
					out_row[k * 4 + 2] = in_row[k * 4 + 2];
					out_row[k * 4 + 3] = in_row[k * 4 + 3];
				}
			}

			for(int j = y2; j < h; j++)
			{
				uint16_t *in_row = (uint16_t*)incoming->get_row_ptr(j);
				uint16_t *out_row = (uint16_t*)outgoing->get_row_ptr(j);

				for(int k = 0; k < w; k++) \
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

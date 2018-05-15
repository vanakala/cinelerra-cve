
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
#include "edl.inc"
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
 : BC_Radial(x, 
		y, 
		plugin->direction == 0, 
		_("In"))
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
	return 0;
}

IrisSquareOut::IrisSquareOut(IrisSquareMain *plugin, 
	IrisSquareWindow *window,
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

int IrisSquareOut::handle_event()
{
	update(1);
	plugin->direction = 1;
	window->in->update(0);
	plugin->send_configure_change();
	return 0;
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
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("IRISSQUARE");
	output.tag.set_property("DIRECTION", direction);
	output.append_tag();
	output.tag.set_title("/IRISSQUARE");
	output.append_tag();
	output.terminate_string();
}

void IrisSquareMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

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
	read_data(prev_keyframe_pts(source_pts));
	return 0;
}


#define IRISSQUARE(type, components) \
{ \
	if(direction == 0) \
	{ \
		int x1 = w / 2 - dw; \
		int x2 = w / 2 + dw; \
		int y1 = h / 2 - dh; \
		int y2 = h / 2 + dh; \
 \
		for(int j = y1; j < y2; j++) \
		{ \
			type *in_row = (type*)incoming->get_row_ptr(j); \
			type *out_row = (type*)outgoing->get_row_ptr(j); \
 \
			for(int k = x1; k < x2; k++) \
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
		int x1 = dw; \
		int x2 = w - dw; \
		int y1 = dh; \
		int y2 = h -dh; \
 \
		for(int j = 0; j < y1; j++) \
		{ \
			type *in_row = (type*)incoming->get_row_ptr(j); \
			type *out_row = (type*)outgoing->get_row_ptr(j); \
 \
			for(int k = 0; k < w; k++) \
			{ \
				out_row[k * components + 0] = in_row[k * components + 0]; \
				out_row[k * components + 1] = in_row[k * components + 1]; \
				out_row[k * components + 2] = in_row[k * components + 2]; \
				if(components == 4) out_row[k * components + 3] = in_row[k * components + 3]; \
			} \
		} \
		for(int j = y1; j < y2; j++) \
		{ \
			type *in_row = (type*)incoming->get_row_ptr(j); \
			type *out_row = (type*)outgoing->get_row_ptr(j); \
 \
			for(int k = 0; k < x1; k++) \
			{ \
				out_row[k * components + 0] = in_row[k * components + 0]; \
				out_row[k * components + 1] = in_row[k * components + 1]; \
				out_row[k * components + 2] = in_row[k * components + 2]; \
				if(components == 4) out_row[k * components + 3] = in_row[k * components + 3]; \
			} \
			for(int k = x2; k < w; k++) \
			{ \
				out_row[k * components + 0] = in_row[k * components + 0]; \
				out_row[k * components + 1] = in_row[k * components + 1]; \
				out_row[k * components + 2] = in_row[k * components + 2]; \
				if(components == 4) out_row[k * components + 3] = in_row[k * components + 3]; \
			} \
		} \
		for(int j = y2; j < h; j++) \
		{ \
			type *in_row = (type*)incoming->get_row_ptr(j); \
			type *out_row = (type*)outgoing->get_row_ptr(j); \
 \
			for(int k = 0; k < w; k++) \
			{ \
				out_row[k * components + 0] = in_row[k * components + 0]; \
				out_row[k * components + 1] = in_row[k * components + 1]; \
				out_row[k * components + 2] = in_row[k * components + 2]; \
				if(components == 4) out_row[k * components + 3] = in_row[k * components + 3]; \
			} \
		} \
	} \
}


void IrisSquareMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	load_configuration();

	int w = incoming->get_w();
	int h = incoming->get_h();
	int dw = round(w / 2 * source_pts / total_len_pts);
	int dh = round(h / 2 * source_pts / total_len_pts);

	switch(incoming->get_color_model())
	{
	case BC_RGB_FLOAT:
		IRISSQUARE(float, 3);
		break;
	case BC_RGB888:
	case BC_YUV888:
		IRISSQUARE(unsigned char, 3)
		break;
	case BC_RGBA_FLOAT:
		IRISSQUARE(float, 4);
		break;
	case BC_RGBA8888:
	case BC_YUVA8888:
		IRISSQUARE(unsigned char, 4)
		break;
	case BC_RGB161616:
	case BC_YUV161616:
		IRISSQUARE(uint16_t, 3)
		break;
	case BC_RGBA16161616:
	case BC_YUVA16161616:
	case BC_AYUV16161616:
		IRISSQUARE(uint16_t, 4)
		break;
	}
}

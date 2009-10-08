
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
#include "edl.inc"
#include "filexml.h"
#include "language.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "vframe.h"
#include "irissquare.h"


#include <stdint.h>
#include <string.h>


REGISTER_PLUGIN(IrisSquareMain)





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
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	320, 
	50, 
	320, 
	50, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}


int IrisSquareWindow::close_event()
{
	set_done(1);
	return 1;
}

void IrisSquareWindow::create_objects()
{
	int x = 10, y = 10;
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
	show_window();
	flush();
}




PLUGIN_THREAD_OBJECT(IrisSquareMain, IrisSquareThread, IrisSquareWindow)






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

char* IrisSquareMain::plugin_title() { return N_("IrisSquare"); }
int IrisSquareMain::is_video() { return 1; }
int IrisSquareMain::is_transition() { return 1; }
int IrisSquareMain::uses_gui() { return 1; }

SHOW_GUI_MACRO(IrisSquareMain, IrisSquareThread);
SET_STRING_MACRO(IrisSquareMain)
RAISE_WINDOW_MACRO(IrisSquareMain)


VFrame* IrisSquareMain::new_picon()
{
	return new VFrame(picon_png);
}

int IrisSquareMain::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sirissquare.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	direction = defaults->get("DIRECTION", direction);
	return 0;
}

int IrisSquareMain::save_defaults()
{
	defaults->update("DIRECTION", direction);
	defaults->save();
	return 0;
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

void IrisSquareMain::load_configuration()
{
	read_data(get_prev_keyframe(get_source_position()));
}






#define IRISSQUARE(type, components) \
{ \
	if(direction == 0) \
	{ \
		int x1 = w / 2 - w / 2 *  \
			PluginClient::get_source_position() /  \
			PluginClient::get_total_len(); \
		int x2 = w / 2 + w / 2 *  \
			PluginClient::get_source_position() /  \
			PluginClient::get_total_len(); \
		int y1 = h / 2 - h / 2 *  \
			PluginClient::get_source_position() /  \
			PluginClient::get_total_len(); \
		int y2 = h / 2 + h / 2 *  \
			PluginClient::get_source_position() /  \
			PluginClient::get_total_len(); \
		for(int j = y1; j < y2; j++) \
		{ \
			type *in_row = (type*)incoming->get_rows()[j]; \
			type *out_row = (type*)outgoing->get_rows()[j]; \
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
		int x1 = w / 2 *  \
			PluginClient::get_source_position() /  \
			PluginClient::get_total_len(); \
		int x2 = w - w / 2 *  \
			PluginClient::get_source_position() /  \
			PluginClient::get_total_len(); \
		int y1 = h / 2 *  \
			PluginClient::get_source_position() /  \
			PluginClient::get_total_len(); \
		int y2 = h - h / 2 *  \
			PluginClient::get_source_position() /  \
			PluginClient::get_total_len(); \
		for(int j = 0; j < y1; j++) \
		{ \
			type *in_row = (type*)incoming->get_rows()[j]; \
			type *out_row = (type*)outgoing->get_rows()[j]; \
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
			type *in_row = (type*)incoming->get_rows()[j]; \
			type *out_row = (type*)outgoing->get_rows()[j]; \
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
			type *in_row = (type*)incoming->get_rows()[j]; \
			type *out_row = (type*)outgoing->get_rows()[j]; \
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





int IrisSquareMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	load_configuration();

	int w = incoming->get_w();
	int h = incoming->get_h();


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
			IRISSQUARE(uint16_t, 4)
			break;
	}
	return 0;
}

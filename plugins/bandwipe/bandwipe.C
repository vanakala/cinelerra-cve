
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

#include "bandwipe.h"
#include "bcdisplayinfo.h"
#include "bchash.h"
#include "edl.inc"
#include "filexml.h"
#include "language.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "vframe.h"


#include <stdint.h>
#include <string.h>






REGISTER_PLUGIN(BandWipeMain)





BandWipeCount::BandWipeCount(BandWipeMain *plugin, 
	BandWipeWindow *window,
	int x,
	int y)
 : BC_TumbleTextBox(window, 
		(int64_t)plugin->bands,
		(int64_t)0,
		(int64_t)1000,
		x, 
		y, 
		50)
{
	this->plugin = plugin;
	this->window = window;
}

int BandWipeCount::handle_event()
{
	plugin->bands = atol(get_text());
	plugin->send_configure_change();
	return 0;
}


BandWipeIn::BandWipeIn(BandWipeMain *plugin, 
	BandWipeWindow *window,
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

int BandWipeIn::handle_event()
{
	update(1);
	plugin->direction = 0;
	window->out->update(0);
	plugin->send_configure_change();
	return 0;
}

BandWipeOut::BandWipeOut(BandWipeMain *plugin, 
	BandWipeWindow *window,
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

int BandWipeOut::handle_event()
{
	update(1);
	plugin->direction = 1;
	window->in->update(0);
	plugin->send_configure_change();
	return 0;
}







BandWipeWindow::BandWipeWindow(BandWipeMain *plugin, int x, int y)
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


int BandWipeWindow::close_event()
{
	set_done(1);
	return 1;
}

void BandWipeWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, _("Bands:")));
	x += 50;
	count = new BandWipeCount(plugin, 
		this,
		x,
		y);
	count->create_objects();
// 	y += 30;
// 	add_subwindow(new BC_Title(x, y, _("Direction:")));
// 	x += 100;
// 	add_subwindow(in = new BandWipeIn(plugin, 
// 		this,
// 		x,
// 		y));
// 	x += 100;
// 	x = 10;
// 	add_subwindow(out = new BandWipeOut(plugin, 
// 		this,
// 		x,
// 		y));
	
	show_window();
	flush();
}




PLUGIN_THREAD_OBJECT(BandWipeMain, BandWipeThread, BandWipeWindow)






BandWipeMain::BandWipeMain(PluginServer *server)
 : PluginVClient(server)
{
	bands = 9;
	direction = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

BandWipeMain::~BandWipeMain()
{
	PLUGIN_DESTRUCTOR_MACRO
}

char* BandWipeMain::plugin_title() { return N_("BandWipe"); }
int BandWipeMain::is_video() { return 1; }
int BandWipeMain::is_transition() { return 1; }
int BandWipeMain::uses_gui() { return 1; }

SHOW_GUI_MACRO(BandWipeMain, BandWipeThread);
SET_STRING_MACRO(BandWipeMain)
RAISE_WINDOW_MACRO(BandWipeMain)


VFrame* BandWipeMain::new_picon()
{
	return new VFrame(picon_png);
}

int BandWipeMain::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sbandwipe.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	bands = defaults->get("BANDS", bands);
	direction = defaults->get("DIRECTION", direction);
	return 0;
}

int BandWipeMain::save_defaults()
{
	defaults->update("BANDS", bands);
	defaults->update("DIRECTION", direction);
	defaults->save();
	return 0;
}

void BandWipeMain::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("BANDWIPE");
	output.tag.set_property("BANDS", bands);
	output.tag.set_property("DIRECTION", direction);
	output.append_tag();
	output.tag.set_title("/BANDWIPE");
	output.append_tag();
	output.terminate_string();
}

void BandWipeMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	while(!input.read_tag())
	{
		if(input.tag.title_is("BANDWIPE"))
		{
			bands = input.tag.get_property("BANDS", bands);
			direction = input.tag.get_property("DIRECTION", direction);
		}
	}
}

void BandWipeMain::load_configuration()
{
	read_data(get_prev_keyframe(get_source_position()));
}



#define BANDWIPE(type, components) \
{ \
	if(direction == 0) \
	{ \
		int x = w * \
			PluginClient::get_source_position() / \
			PluginClient::get_total_len(); \
 \
		for(int i = 0; i < bands; i++) \
		{ \
			for(int j = 0; j < band_h; j++) \
			{ \
				int row = i * band_h + j; \
				 \
				if(row >= 0 && row < h) \
				{ \
					type *in_row = (type*)incoming->get_rows()[row]; \
					type *out_row = (type*)outgoing->get_rows()[row]; \
 \
					if(i % 2) \
					{ \
						for(int k = 0; k < x; k++) \
						{ \
							out_row[k * components + 0] = in_row[k * components + 0]; \
							out_row[k * components + 1] = in_row[k * components + 1]; \
							out_row[k * components + 2] = in_row[k * components + 2]; \
							if(components == 4) out_row[k * components + 3] = in_row[k * components + 3]; \
						} \
					} \
					else \
					{ \
						for(int k = w - x; k < w; k++) \
						{ \
							out_row[k * components + 0] = in_row[k * components + 0]; \
							out_row[k * components + 1] = in_row[k * components + 1]; \
							out_row[k * components + 2] = in_row[k * components + 2]; \
							if(components == 4) out_row[k * components + 3] = in_row[k * components + 3]; \
						} \
					} \
				} \
			} \
		} \
	} \
	else \
	{ \
		int x = w - w * \
			PluginClient::get_source_position() / \
			PluginClient::get_total_len(); \
 \
		for(int i = 0; i < bands; i++) \
		{ \
			for(int j = 0; j < band_h; j++) \
			{ \
				int row = i * band_h + j; \
				 \
				if(row >= 0 && row < h) \
				{ \
					type *in_row = (type*)incoming->get_rows()[row]; \
					type *out_row = (type*)outgoing->get_rows()[row]; \
 \
					if(i % 2) \
					{ \
						for(int k = x; k < w; k++) \
						{ \
							out_row[k * components + 0] = in_row[k * components + 0]; \
							out_row[k * components + 1] = in_row[k * components + 1]; \
							out_row[k * components + 2] = in_row[k * components + 2]; \
							if(components == 4) out_row[k * components + 3] = in_row[k * components + 3]; \
						} \
					} \
					else \
					{ \
						for(int k = 0; k < w - x; k++) \
						{ \
							out_row[k * components + 0] = in_row[k * components + 0]; \
							out_row[k * components + 1] = in_row[k * components + 1]; \
							out_row[k * components + 2] = in_row[k * components + 2]; \
							if(components == 4) out_row[k * components + 3] = in_row[k * components + 3]; \
						} \
					} \
				} \
			} \
		} \
	} \
}



int BandWipeMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	load_configuration();

	int w = incoming->get_w();
	int h = incoming->get_h();
	int band_h = ((bands == 0) ? h : (h / bands + 1));

	switch(incoming->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			BANDWIPE(unsigned char, 3)
			break;
		case BC_RGB_FLOAT:
			BANDWIPE(float, 3);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			BANDWIPE(unsigned char, 4)
			break;
		case BC_RGBA_FLOAT:
			BANDWIPE(float, 4);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			BANDWIPE(uint16_t, 3)
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			BANDWIPE(uint16_t, 4)
			break;
	}

	return 0;
}

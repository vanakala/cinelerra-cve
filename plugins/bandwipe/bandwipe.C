// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "bctextbox.h"
#include "bandwipe.h"
#include "bchash.h"
#include "edl.inc"
#include "filexml.h"
#include "picon_png.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>

REGISTER_PLUGIN


BandWipeCount::BandWipeCount(BandWipeMain *plugin, 
	BandWipeWindow *window,
	int x,
	int y)
 : BC_TumbleTextBox(window, plugin->bands,
	0, 1000, x, y, 50)
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

BandWipeWindow::BandWipeWindow(BandWipeMain *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y, 
	320, 
	50)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Bands:")));
	x += 50;
	count = new BandWipeCount(plugin, 
		this,
		x,
		y);
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

PLUGIN_THREAD_METHODS


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

PLUGIN_CLASS_METHODS

void BandWipeMain::load_defaults()
{
	defaults = load_defaults_file("bandwipe.rc");

	bands = defaults->get("BANDS", bands);
	direction = defaults->get("DIRECTION", direction);
}

void BandWipeMain::save_defaults()
{
	defaults->update("BANDS", bands);
	defaults->update("DIRECTION", direction);
	defaults->save();
}

void BandWipeMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("BANDWIPE");
	output.tag.set_property("BANDS", bands);
	output.tag.set_property("DIRECTION", direction);
	output.append_tag();
	output.tag.set_title("/BANDWIPE");
	output.append_tag();
	keyframe->set_data(output.string);
}

void BandWipeMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	if(!keyframe)
		return;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("BANDWIPE"))
		{
			bands = input.tag.get_property("BANDS", bands);
			direction = input.tag.get_property("DIRECTION", direction);
		}
	}
}

int BandWipeMain::load_configuration()
{
	read_data(prev_keyframe_pts(source_pts));
	return 0;
}

#define BANDWIPE(type, components) \
{ \
	if(direction == 0) \
	{ \
		int x = round(w * source_pts / get_length()); \
 \
		for(int i = 0; i < bands; i++) \
		{ \
			for(int j = 0; j < band_h; j++) \
			{ \
				int row = i * band_h + j; \
				 \
				if(row >= 0 && row < h) \
				{ \
					type *in_row = (type*)incoming->get_row_ptr(row); \
					type *out_row = (type*)outgoing->get_row_ptr(row); \
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
		int x = w - (int)(round(w * source_pts / get_length())); \
 \
		for(int i = 0; i < bands; i++) \
		{ \
			for(int j = 0; j < band_h; j++) \
			{ \
				int row = i * band_h + j; \
				 \
				if(row >= 0 && row < h) \
				{ \
					type *in_row = (type*)incoming->get_row_ptr(row); \
					type *out_row = (type*)outgoing->get_row_ptr(row); \
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



void BandWipeMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	load_configuration();

	int w = incoming->get_w();
	int h = incoming->get_h();
	int band_h = ((bands == 0) ? h : (h / bands + 1));

	if(get_length() < EPSILON)
		return;

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
		case BC_AYUV16161616:
			BANDWIPE(uint16_t, 4)
			break;
	}
}

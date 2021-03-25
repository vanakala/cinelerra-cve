// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bchash.h"
#include "bctitle.h"
#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "shiftinterlace.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>

REGISTER_PLUGIN

ShiftInterlaceConfig::ShiftInterlaceConfig()
{
	odd_offset = 0;
	even_offset = 0;
}

int ShiftInterlaceConfig::equivalent(ShiftInterlaceConfig &that)
{
	return (odd_offset == that.odd_offset &&
		even_offset == that.even_offset);
}

void ShiftInterlaceConfig::copy_from(ShiftInterlaceConfig &that)
{
	odd_offset = that.odd_offset;
	even_offset = that.even_offset;
}

void ShiftInterlaceConfig::interpolate(ShiftInterlaceConfig &prev, 
		ShiftInterlaceConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->odd_offset = round(prev.odd_offset * prev_scale + next.odd_offset * next_scale);
	this->even_offset = round(prev.even_offset * prev_scale + next.even_offset * next_scale);
}


ShiftInterlaceWindow::ShiftInterlaceWindow(ShiftInterlaceMain *plugin, 
	int x, 
	int y)
 : PluginWindow(plugin,
	x,
	y,
	310, 
	100)
{
	int margin = 30;
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Odd offset:")));
	add_subwindow(odd_offset = new ShiftInterlaceOdd(plugin, x + 90, y));
	y += margin;
	add_subwindow(new BC_Title(x, y, _("Even offset:")));
	add_subwindow(even_offset = new ShiftInterlaceEven(plugin, x + 90, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void ShiftInterlaceWindow::update()
{
	odd_offset->update(plugin->config.odd_offset);
	even_offset->update(plugin->config.even_offset);
}


ShiftInterlaceOdd::ShiftInterlaceOdd(ShiftInterlaceMain *plugin, int x, int y)
 : BC_ISlider(x,
	y,
	0,
	200,
	200,
	-100,
	100,
	plugin->config.odd_offset)
{
	this->plugin = plugin;
}

int ShiftInterlaceOdd::handle_event()
{
	plugin->config.odd_offset = get_value();
	plugin->send_configure_change();
	return 1;
}


ShiftInterlaceEven::ShiftInterlaceEven(ShiftInterlaceMain *plugin, int x, int y)
 : BC_ISlider(x,
	y,
	0,
	200,
	200,
	-100,
	100,
	plugin->config.even_offset)
{
	this->plugin = plugin;
}

int ShiftInterlaceEven::handle_event()
{
	plugin->config.even_offset = get_value();
	plugin->send_configure_change();
	return 1;
}


PLUGIN_THREAD_METHODS

ShiftInterlaceMain::ShiftInterlaceMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

ShiftInterlaceMain::~ShiftInterlaceMain()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void ShiftInterlaceMain::load_defaults()
{
	defaults = load_defaults_file("shiftinterlace.rc");

	config.odd_offset = defaults->get("ODD_OFFSET", config.odd_offset);
	config.even_offset = defaults->get("EVEN_OFFSET", config.even_offset);
}

void ShiftInterlaceMain::save_defaults()
{
	defaults->update("ODD_OFFSET", config.odd_offset);
	defaults->update("EVEN_OFFSET", config.even_offset);
	defaults->save();
}

void ShiftInterlaceMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("SHIFTINTERLACE");
	output.tag.set_property("ODD_OFFSET", config.odd_offset);
	output.tag.set_property("EVEN_OFFSET", config.even_offset);
	output.append_tag();
	output.tag.set_title("/SHIFTINTERLACE");
	output.append_tag();
	keyframe->set_data(output.string);
}

void ShiftInterlaceMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("SHIFTINTERLACE"))
		{
			config.odd_offset = input.tag.get_property("ODD_OFFSET", config.odd_offset);
			config.even_offset = input.tag.get_property("EVEN_OFFSET", config.even_offset);
		}
	}
}

#define SHIFT_ROW_MACRO(components, type, chroma_offset) \
{ \
	type *input_row = (type*)input_frame->get_row_ptr(row); \
	type *output_row = (type*)input_frame->get_row_ptr(row); \
 \
	if(offset < 0) \
	{ \
		int i, j; \
		for(i = 0, j = -offset;  \
			j < w;  \
			i++, j++) \
		{ \
			output_row[i * components + 0] = input_row[j * components + 0]; \
			output_row[i * components + 1] = input_row[j * components + 1]; \
			output_row[i * components + 2] = input_row[j * components + 2]; \
			if(components == 4) output_row[i * components + 3] = input_row[j * components + 3]; \
		} \
 \
		for( ; i < w; i++) \
		{ \
			output_row[i * components + 0] = 0; \
			output_row[i * components + 1] = chroma_offset; \
			output_row[i * components + 2] = chroma_offset; \
			if(components == 4) output_row[i * components + 3] = 0; \
		} \
	} \
	else \
	{ \
		int i, j; \
		for(i = w - offset - 1, j = w - 1; \
			j >= offset; \
			i--, \
			j--) \
		{ \
			output_row[j * components + 0] = input_row[i * components + 0]; \
			output_row[j * components + 1] = input_row[i * components + 1]; \
			output_row[j * components + 2] = input_row[i * components + 2]; \
			if(components == 4) output_row[j * components + 3] = input_row[i * components + 3]; \
		} \
 \
		for( ; j >= 0; j--) \
		{ \
			output_row[j * components + 0] = 0; \
			output_row[j * components + 1] = chroma_offset; \
			output_row[j * components + 2] = chroma_offset; \
			if(components == 4) output_row[j * components + 3] = 0; \
		} \
	} \
}

void ShiftInterlaceMain::shift_row(VFrame *input_frame, int offset, int row)
{
	int i, j;
	int w = input_frame->get_w();
	uint16_t *input_row = (uint16_t*)input_frame->get_row_ptr(row);
	uint16_t *output_row = (uint16_t*)input_frame->get_row_ptr(row);

	switch(input_frame->get_color_model())
	{
	case BC_RGBA16161616:
/* Pole
		SHIFT_ROW_MACRO(4, uint16_t, 0x0)
	#define SHIFT_ROW_MACRO(components, type, chroma_offset) \
{ \
	type *input_row = (type*)input_frame->get_row_ptr(row); \
	type *output_row = (type*)input_frame->get_row_ptr(row); \
 \
	*/
		if(offset < 0)
		{
			for(i = 0, j = -offset; j < w; i++, j++)
			{
				output_row[i * 4 + 0] = input_row[j * 4 + 0];
				output_row[i * 4 + 1] = input_row[j * 4 + 1];
				output_row[i * 4 + 2] = input_row[j * 4 + 2];
				output_row[i * 4 + 3] = input_row[j * 4 + 3];
			}

			for(; i < w; i++)
			{
				output_row[i * 4 + 0] = 0;
				output_row[i * 4 + 1] = 0;
				output_row[i * 4 + 2] = 0;
				output_row[i * 4 + 3] = 0;
			}
		}
		else
		{
			for(i = w - offset - 1, j = w - 1; j >= offset; i--, j--)
			{
				output_row[j * 4 + 0] = input_row[i * 4 + 0];
				output_row[j * 4 + 1] = input_row[i * 4 + 1];
				output_row[j * 4 + 2] = input_row[i * 4 + 2];
				output_row[j * 4 + 3] = input_row[i * 4 + 3];
			}

			for(; j >= 0; j--)
			{
				output_row[j * 4 + 0] = 0;
				output_row[j * 4 + 1] = 0;
				output_row[j * 4 + 2] = 0;
				output_row[j * 4 + 3] = 0;
			}
		}
		break;
	case BC_AYUV16161616:
		if(offset < 0)
		{
			for(i = 0, j = -offset; j < w; i++, j++)
			{
				output_row[i * 4 + 0] = input_row[j * 4 + 0];
				output_row[i * 4 + 1] = input_row[j * 4 + 1];
				output_row[i * 4 + 2] = input_row[j * 4 + 2];
				output_row[i * 4 + 3] = input_row[j * 4 + 3];
			}

			for(; i < w; i++)
			{
				output_row[i * 4 + 0] = 0;
				output_row[i * 4 + 1] = 0;
				output_row[i * 4 + 2] = 0x8000;
				output_row[i * 4 + 3] = 0x8000;
			}
		}
		else
		{
			for(i = w - offset - 1, j = w - 1; j >= offset; i--, j--)
			{
				output_row[j * 4 + 0] = input_row[i * 4 + 0];
				output_row[j * 4 + 1] = input_row[i * 4 + 1];
				output_row[j * 4 + 2] = input_row[i * 4 + 2];
				output_row[j * 4 + 3] = input_row[i * 4 + 3];
			}

			for(; j >= 0; j--)
			{
				output_row[j * 4 + 0] = 0;
				output_row[j * 4 + 1] = 0;
				output_row[j * 4 + 2] = 0x8000;
				output_row[j * 4 + 3] = 0x8000;
			}
		}
		break;
	}
}

VFrame *ShiftInterlaceMain::process_tmpframe(VFrame *input)
{
	int h = input->get_h();
	int color_model = input->get_color_model();

	switch(color_model)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(color_model);
		return input;
	}

	if(load_configuration())
		update_gui();

	for(int i = 0; i < h; i++)
	{
		if(i % 2)
			shift_row(input, config.even_offset, i);
		else
			shift_row(input, config.odd_offset, i);
	}
	return input;
}

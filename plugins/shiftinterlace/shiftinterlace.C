
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

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME

#define PLUGIN_TITLE N_("ShiftInterlace")
#define PLUGIN_CLASS ShiftInterlaceMain
#define PLUGIN_CONFIG_CLASS ShiftInterlaceConfig
#define PLUGIN_THREAD_CLASS ShiftInterlaceThread
#define PLUGIN_GUI_CLASS ShiftInterlaceWindow

#include "pluginmacros.h"

#include "bchash.h"
#include "bcslider.h"
#include "bctitle.h"
#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.h"


#include <stdint.h>
#include <string.h>


class ShiftInterlaceConfig
{
public:
	ShiftInterlaceConfig();

	int equivalent(ShiftInterlaceConfig &that);
	void copy_from(ShiftInterlaceConfig &that);
	void interpolate(ShiftInterlaceConfig &prev, 
		ShiftInterlaceConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int odd_offset;
	int even_offset;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class ShiftInterlaceOdd : public BC_ISlider
{
public:
	ShiftInterlaceOdd(ShiftInterlaceMain *plugin, int x, int y);
	int handle_event();
	ShiftInterlaceMain *plugin;
};

class ShiftInterlaceEven : public BC_ISlider
{
public:
	ShiftInterlaceEven(ShiftInterlaceMain *plugin, int x, int y);
	int handle_event();
	ShiftInterlaceMain *plugin;
};

class ShiftInterlaceWindow : public PluginWindow
{
public:
	ShiftInterlaceWindow(ShiftInterlaceMain *plugin, int x, int y);

	void update();

	ShiftInterlaceOdd *odd_offset;
	ShiftInterlaceEven *even_offset;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class ShiftInterlaceMain : public PluginVClient
{
public:
	ShiftInterlaceMain(PluginServer *server);
	~ShiftInterlaceMain();

	PLUGIN_CLASS_MEMBERS

// required for all realtime plugins
	void process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();

	void shift_row(VFrame *input_frame, 
		VFrame *output_frame,
		int offset,
		int row);
};


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
 : PluginWindow(plugin->gui_string, 
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

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("SHIFTINTERLACE");
	output.tag.set_property("ODD_OFFSET", config.odd_offset);
	output.tag.set_property("EVEN_OFFSET", config.even_offset);
	output.append_tag();
	output.tag.set_title("/SHIFTINTERLACE");
	output.append_tag();
	output.append_newline();
// data is now in *text
}

void ShiftInterlaceMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

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
	type *output_row = (type*)output_frame->get_row_ptr(row); \
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

void ShiftInterlaceMain::shift_row(VFrame *input_frame, 
	VFrame *output_frame,
	int offset,
	int row)
{
	int w = input_frame->get_w();
	switch(input_frame->get_color_model())
	{
	case BC_RGB888:
		SHIFT_ROW_MACRO(3, unsigned char, 0x0)
		break;
	case BC_RGB_FLOAT:
		SHIFT_ROW_MACRO(3, float, 0x0)
		break;
	case BC_YUV888:
		SHIFT_ROW_MACRO(3, unsigned char, 0x80)
		break;
	case BC_RGBA_FLOAT:
		SHIFT_ROW_MACRO(4, float, 0x0)
		break;
	case BC_RGBA8888:
		SHIFT_ROW_MACRO(4, unsigned char, 0x0)
		break;
	case BC_YUVA8888:
		SHIFT_ROW_MACRO(4, unsigned char, 0x80)
		break;
	case BC_RGB161616:
		SHIFT_ROW_MACRO(3, uint16_t, 0x0)
		break;
	case BC_YUV161616:
		SHIFT_ROW_MACRO(3, uint16_t, 0x8000)
		break;
	case BC_RGBA16161616:
		SHIFT_ROW_MACRO(4, uint16_t, 0x0)
		break;
	case BC_YUVA16161616:
		SHIFT_ROW_MACRO(4, uint16_t, 0x8000)
		break;
	case BC_AYUV16161616:
/* Pole
		SHIFT_ROW_MACRO(4, uint16_t, 0x8000)
	#define SHIFT_ROW_MACRO(components, type, chroma_offset) \
	*/
		{
			uint16_t *input_row = (uint16_t*)input_frame->get_row_ptr(row);
			uint16_t *output_row = (uint16_t*)output_frame->get_row_ptr(row);

			if(offset < 0)
			{
				int i, j;
				for(i = 0, j = -offset; j < w; i++, j++)
				{
					output_row[i * 4 + 0] = input_row[j * 4 + 0];
					output_row[i * 4 + 1] = input_row[j * 4 + 1];
					output_row[i * 4 + 2] = input_row[j * 4 + 2];
					output_row[i * 4 + 3] = input_row[j * 4 + 3];
				}

				for( ; i < w; i++)
				{
					output_row[i * 4 + 0] = 0;
					output_row[i * 4 + 1] = 0;
					output_row[i * 4 + 2] = 0x8000;
					output_row[i * 4 + 3] = 0x8000;
				}
			}
			else
			{
				int i, j;
				for(i = w - offset - 1, j = w - 1;
					j >= offset; i--, j--)
				{
					output_row[j * 4 + 0] = input_row[i * 4 + 0];
					output_row[j * 4 + 1] = input_row[i * 4 + 1];
					output_row[j * 4 + 2] = input_row[i * 4 + 2];
					output_row[j * 4 + 3] = input_row[i * 4 + 3];
				}

				for( ; j >= 0; j--)
				{
					output_row[j * 4 + 0] = 0;
					output_row[j * 4 + 1] = 0;
					output_row[j * 4 + 2] = 0x8000;
					output_row[j * 4 + 3] = 0x8000;
				}
			}
		}
		break;
	}
}

void ShiftInterlaceMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	load_configuration();

	int h = input_ptr->get_h();
	for(int i = 0; i < h; i++)
	{
		if(i % 2)
			shift_row(input_ptr, output_ptr, config.even_offset, i);
		else
			shift_row(input_ptr, output_ptr, config.odd_offset, i);
	}
}

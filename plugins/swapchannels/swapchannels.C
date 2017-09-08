
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

#include "bctitle.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "swapchannels.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>


REGISTER_PLUGIN


SwapConfig::SwapConfig()
{
	red = RED_SRC;
	green = GREEN_SRC;
	blue = BLUE_SRC;
	alpha = ALPHA_SRC;
}

int SwapConfig::equivalent(SwapConfig &that)
{
	return (red == that.red &&
		green == that.green &&
		blue == that.blue &&
		alpha == that.alpha);
}

void SwapConfig::copy_from(SwapConfig &that)
{
	red = that.red;
	green = that.green;
	blue = that.blue;
	alpha = that.alpha;
}


SwapWindow::SwapWindow(SwapMain *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y,
	250, 
	170)
{
	int margin = 30;
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _(plugin->plugin_title())));
	y += margin;
	add_subwindow(new BC_Title(x + 160, y + 5, _("-> Red")));
	add_subwindow(red = new SwapMenu(plugin, &(plugin->config.red), x, y));
	red->create_objects();
	y += margin;
	add_subwindow(new BC_Title(x + 160, y + 5, _("-> Green")));
	add_subwindow(green = new SwapMenu(plugin, &(plugin->config.green), x, y));
	green->create_objects();
	y += margin;
	add_subwindow(new BC_Title(x + 160, y + 5, _("-> Blue")));
	add_subwindow(blue = new SwapMenu(plugin, &(plugin->config.blue), x, y));
	blue->create_objects();
	y += margin;
	add_subwindow(new BC_Title(x + 160, y + 5, _("-> Alpha")));
	add_subwindow(alpha = new SwapMenu(plugin, &(plugin->config.alpha), x, y));
	alpha->create_objects();
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

SwapWindow::~SwapWindow()
{
}

void SwapWindow::update()
{
	red->set_text(plugin->output_to_text(plugin->config.red));
	green->set_text(plugin->output_to_text(plugin->config.green));
	blue->set_text(plugin->output_to_text(plugin->config.blue));
	alpha->set_text(plugin->output_to_text(plugin->config.alpha));
}

SwapMenu::SwapMenu(SwapMain *client, int *output, int x, int y)
 : BC_PopupMenu(x, y, 150, client->output_to_text(*output))
{
	this->client = client;
	this->output = output;
}

int SwapMenu::handle_event()
{
	client->send_configure_change();
	return 1;
}

void SwapMenu::create_objects()
{
	add_item(new SwapItem(this, client->output_to_text(RED_SRC)));
	add_item(new SwapItem(this, client->output_to_text(GREEN_SRC)));
	add_item(new SwapItem(this, client->output_to_text(BLUE_SRC)));
	add_item(new SwapItem(this, client->output_to_text(ALPHA_SRC)));
	add_item(new SwapItem(this, client->output_to_text(NO_SRC)));
	add_item(new SwapItem(this, client->output_to_text(MAX_SRC)));
}


SwapItem::SwapItem(SwapMenu *menu, const char *title)
 : BC_MenuItem(title)
{
	this->menu = menu;
}

int SwapItem::handle_event()
{
	menu->set_text(get_text());
	*(menu->output) = menu->client->text_to_output(get_text());
	menu->handle_event();
	return 1;
}


PLUGIN_THREAD_METHODS


SwapMain::SwapMain(PluginServer *server)
 : PluginVClient(server)
{
	temp = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

SwapMain::~SwapMain()
{
	if(temp) delete temp;
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void SwapMain::load_defaults()
{
	defaults = load_defaults_file("swapchannels.rc");

	config.red = defaults->get("RED", config.red);
	config.green = defaults->get("GREEN", config.green);
	config.blue = defaults->get("BLUE", config.blue);
	config.alpha = defaults->get("ALPHA", config.alpha);
}

void SwapMain::save_defaults()
{
	defaults->update("RED", config.red);
	defaults->update("GREEN", config.green);
	defaults->update("BLUE", config.blue);
	defaults->update("ALPHA", config.alpha);
	defaults->save();
}

void SwapMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("SWAPCHANNELS");
	output.tag.set_property("RED", config.red);
	output.tag.set_property("GREEN", config.green);
	output.tag.set_property("BLUE", config.blue);
	output.tag.set_property("ALPHA", config.alpha);
	output.append_tag();
	output.tag.set_title("/SWAPCHANNELS");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
// data is now in *text
}

void SwapMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	while(!input.read_tag())
	{
		if(input.tag.title_is("SWAPCHANNELS"))
		{
			config.red = input.tag.get_property("RED", config.red);
			config.green = input.tag.get_property("GREEN", config.green);
			config.blue = input.tag.get_property("BLUE", config.blue);
			config.alpha = input.tag.get_property("ALPHA", config.alpha);
		}
	}
}

int SwapMain::load_configuration()
{
	KeyFrame *prev_keyframe;

	prev_keyframe = prev_keyframe_pts(source_pts);
	read_data(prev_keyframe);
	return 1;
}

#define MAXMINSRC(src, max) \
	(src == MAX_SRC ? max : 0)

#define SWAP_CHANNELS(type, max, components) \
{ \
	int h = input_ptr->get_h(); \
	int w = input_ptr->get_w(); \
	int red = config.red; \
	int green = config.green; \
	int blue = config.blue; \
	int alpha = config.alpha; \
 \
	if(components == 3) \
	{ \
		if(red == ALPHA_SRC) red = MAX_SRC; \
		if(green == ALPHA_SRC) green = MAX_SRC; \
		if(blue == ALPHA_SRC) blue = MAX_SRC; \
	} \
 \
	for(int i = 0; i < h; i++) \
	{ \
		type *inrow = (type*)input_ptr->get_rows()[i]; \
		type *outrow = (type*)temp->get_rows()[i]; \
 \
		for(int j = 0; j < w; j++) \
		{ \
			if(red < 4) \
				*outrow++ = *(inrow + red); \
			else \
				*outrow++ = MAXMINSRC(red, max); \
 \
			if(green < 4) \
				*outrow++ = *(inrow + green); \
			else \
				*outrow++ = MAXMINSRC(green, max); \
 \
			if(blue < 4) \
				*outrow++ = *(inrow + blue); \
			else \
				*outrow++ = MAXMINSRC(blue, max); \
 \
			if(components == 4) \
			{ \
				if(alpha < 4) \
					*outrow++ = *(inrow + alpha); \
				else \
					*outrow++ = MAXMINSRC(alpha, max); \
			} \
 \
			inrow += components; \
		} \
	} \
 \
	output_ptr->copy_from(temp); \
}

void SwapMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	load_configuration();

	if(!temp) 
		temp = new VFrame(0, 
			input_ptr->get_w(), 
			input_ptr->get_h(), 
			input_ptr->get_color_model());

	switch(input_ptr->get_color_model())
	{
	case BC_RGB_FLOAT:
		SWAP_CHANNELS(float, 1, 3);
		break;
	case BC_RGBA_FLOAT:
		SWAP_CHANNELS(float, 1, 4);
		break;
	case BC_RGB888:
	case BC_YUV888:
		SWAP_CHANNELS(unsigned char, 0xff, 3);
		break;
	case BC_RGBA8888:
	case BC_YUVA8888:
		SWAP_CHANNELS(unsigned char, 0xff, 4);
		break;
	case BC_RGB161616:
	case BC_YUV161616:
		SWAP_CHANNELS(uint16_t, 0xffff, 3);
		break;
	case BC_RGBA16161616:
	case BC_YUVA16161616:
		SWAP_CHANNELS(uint16_t, 0xffff, 4);
		break;
	}
}


const char* SwapMain::output_to_text(int value)
{
	switch(value)
	{
	case RED_SRC:
		return _("Red");

	case GREEN_SRC:
		return _("Green");

	case BLUE_SRC:
		return _("Blue");

	case ALPHA_SRC:
		return _("Alpha");

	case NO_SRC:
		return _("0%");

	case MAX_SRC:
		return _("100%");

	default:
		return "";
	}
}

int SwapMain::text_to_output(const char *text)
{
	if(!strcmp(text, _("Red"))) return RED_SRC;
	if(!strcmp(text, _("Green"))) return GREEN_SRC;
	if(!strcmp(text, _("Blue"))) return BLUE_SRC;
	if(!strcmp(text, _("Alpha"))) return ALPHA_SRC;
	if(!strcmp(text, _("0%"))) return NO_SRC;
	if(!strcmp(text, _("100%"))) return MAX_SRC;
	return 0;
}


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

#include "720to480.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "bcdisplayinfo.h"
#include "keyframe.h"
#include "language.h"
#include "mainprogress.h"
#include "vframe.h"


#include <stdint.h>
#include <string.h>


REGISTER_PLUGIN(_720to480Main)


#define FORWARD 0
#define REVERSE 1

_720to480Config::_720to480Config()
{
	first_field = 0;
	direction = FORWARD;
}





_720to480Window::_720to480Window(_720to480Main *client, int x, int y)
 : BC_Window(client->gui_string, 
 	x, 
	y, 
	230, 
	150, 
	230, 
	150, 
	0, 
	0,
	1)
{ 
	this->client = client; 
}


_720to480Window::~_720to480Window()
{
}

int _720to480Window::create_objects()
{
	int x = 10, y = 10;

	add_tool(odd_first = new _720to480Order(client, this, 1, x, y, _("Odd field first")));
	y += 25;
	add_tool(even_first = new _720to480Order(client, this, 0, x, y, _("Even field first")));

// 	y += 25;
// 	add_tool(forward = new _720to480Direction(client, this, FORWARD, x, y, _("Downsample")));
// 	y += 25;
// 	add_tool(reverse = new _720to480Direction(client, this, REVERSE, x, y, _("Upsample")));
// 
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));

	show_window();
	flush();
	return 0;
}

WINDOW_CLOSE_EVENT(_720to480Window)

int _720to480Window::set_first_field(int first_field)
{
	odd_first->update(first_field == 1);
	even_first->update(first_field == 0);

	client->config.first_field = first_field;
	return 0;
}

int _720to480Window::set_direction(int direction)
{
	forward->update(direction == FORWARD);
	reverse->update(direction == REVERSE);


	client->config.direction = direction;
	return 0;
}



_720to480Order::_720to480Order(_720to480Main *client, 
		_720to480Window *window, 
		int output, 
		int x, 
		int y, 
		char *text)
 : BC_Radial(x, 
 	y, 
	client->config.first_field == output, 
	text)
{
	this->client = client;
	this->window = window;
	this->output = output;
}

int _720to480Order::handle_event()
{
	window->set_first_field(output);
	return 1;
}





_720to480Direction::_720to480Direction(_720to480Main *client, 
		_720to480Window *window, 
		int output, 
		int x, 
		int y, 
		char *text)
 : BC_Radial(x, 
 	y, 
	client->config.direction == output, 
	text)
{
	this->client = client;
	this->window = window;
	this->output = output;
}

int _720to480Direction::handle_event()
{
	window->set_direction(output);
	return 1;
}
















_720to480Main::_720to480Main(PluginServer *server)
 : PluginVClient(server)
{
	temp = 0;
	load_defaults();
}

_720to480Main::~_720to480Main()
{
	save_defaults();
	delete defaults;

	if(temp) delete temp;
}

char* _720to480Main::plugin_title() { return N_("720 to 480"); }
int _720to480Main::is_realtime() { return 0; }

double _720to480Main::get_framerate()
{
	return project_frame_rate / 2;
}





int _720to480Main::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
	sprintf(directory, "%s720to480.rc", BCASTDIR);
	
	defaults = new BC_Hash(directory);
	defaults->load();
	config.first_field = defaults->get("FIRST_FIELD", config.first_field);
	config.direction = defaults->get("DIRECTION", config.direction);
	return 0;
}


int _720to480Main::save_defaults()
{
	defaults->update("FIRST_FIELD", config.first_field);
	defaults->update("DIRECTION", config.direction);
	defaults->save();
	return 0;
}

int _720to480Main::get_parameters()
{
	BC_DisplayInfo info;
	_720to480Window window(this, 
		info.get_abs_cursor_x(), 
		info.get_abs_cursor_y());
	window.create_objects();
	int result = window.run_window();
	return result;
}

int _720to480Main::start_loop()
{
	if(PluginClient::interactive)
	{
		char string[BCTEXTLEN];
		sprintf(string, "%s...", plugin_title());
		progress = start_progress(string, 
			PluginClient::end - PluginClient::start);
	}

	input_position = PluginClient::start;
	return 0;
}


int _720to480Main::stop_loop()
{
	if(PluginClient::interactive)
	{
		progress->stop_progress();
		delete progress;
	}
	return 0;
}


#define DST_W 854
#define DST_H 240


void _720to480Main::reduce_field(VFrame *output, VFrame *input, int dest_row)
{
	int in_w = input->get_w();
	int in_h = input->get_h();
	int out_w = output->get_w();
	int out_h = output->get_h();

#define REDUCE_MACRO(type, temp, components) \
for(int i = 0; i < DST_H; i++) \
{ \
	int output_number = dest_row + i * 2; \
	if(output_number >= out_h) break; \
 \
	int in1 = i * 3 + dest_row * 2; \
	int in2 = i * 3 + 1 + dest_row * 2; \
	int in3 = i * 3 + 2 + dest_row * 2; \
 \
	if(in1 >= in_h) in1 = in_h - 1; \
	if(in2 >= in_h) in2 = in_h - 1; \
	if(in3 >= in_h) in3 = in_h - 1; \
 \
	type *out_row = (type*)output->get_rows()[output_number]; \
	type *in_row1 = (type*)input->get_rows()[in1]; \
	type *in_row2 = (type*)input->get_rows()[in2]; \
	type *in_row3 = (type*)input->get_rows()[in3]; \
 \
	int w = MIN(out_w, in_w) * components; \
	for(int j = 0; j < w; j++) \
	{ \
		*out_row++ = ((temp)*in_row1++ + \
			(temp)*in_row2++ + \
			(temp)*in_row3++) / 3; \
	} \
}

	switch(input->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			REDUCE_MACRO(unsigned char, int64_t, 3);
			break;
		case BC_RGB_FLOAT:
			REDUCE_MACRO(float, float, 3);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			REDUCE_MACRO(unsigned char, int64_t, 4);
			break;
		case BC_RGBA_FLOAT:
			REDUCE_MACRO(float, float, 4);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			REDUCE_MACRO(uint16_t, int64_t, 3);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			REDUCE_MACRO(uint16_t, int64_t, 4);
			break;
	}
}

int _720to480Main::process_loop(VFrame *output)
{
	int result = 0;

	if(!temp)
		temp = new VFrame(0,
			output->get_w(),
			output->get_h(),
			output->get_color_model());

	if(config.direction == FORWARD)
	{
// Step 1: Reduce vertically and put in desired fields of output
		read_frame(temp, input_position);
		reduce_field(output, temp, config.first_field == 0 ? 0 : 1);
		input_position++;

		read_frame(temp, input_position);
		reduce_field(output, temp, config.first_field == 0 ? 1 : 0);
		input_position++;
	}

	if(PluginClient::interactive) 
		result = progress->update(input_position - PluginClient::start);

	if(input_position >= PluginClient::end) result = 1;

	return result;
}






void _720to480Main::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("720TO480");
	output.tag.set_property("FIRST_FIELD", config.first_field);
	output.tag.set_property("DIRECTION", config.direction);
	output.append_tag();
	output.tag.set_title("/720TO480");
	output.append_tag();
	output.terminate_string();
}

void _720to480Main::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	while(!input.read_tag())
	{
		if(input.tag.title_is("720TO480"))
		{
			config.first_field = input.tag.get_property("FIRST_FIELD", config.first_field);
			config.direction = input.tag.get_property("DIRECTION", config.direction);
		}
	}
}


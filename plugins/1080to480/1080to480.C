#include "1080to480.h"
#include "clip.h"
#include "defaults.h"
#include "filexml.h"
#include "bcdisplayinfo.h"
#include "keyframe.h"
#include "language.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "vframe.h"






#include <stdint.h>
#include <string.h>


REGISTER_PLUGIN(_1080to480Main)




_1080to480Config::_1080to480Config()
{
	first_field = 0;
}

int _1080to480Config::equivalent(_1080to480Config &that)
{
	return first_field == that.first_field;
}

void _1080to480Config::copy_from(_1080to480Config &that)
{
	first_field = that.first_field;
}

void _1080to480Config::interpolate(_1080to480Config &prev, 
	_1080to480Config &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	copy_from(prev);
}





_1080to480Window::_1080to480Window(_1080to480Main *client, int x, int y)
 : BC_Window(client->gui_string, 
 	x, 
	y, 
	200, 
	100, 
	200, 
	100, 
	0, 
	0,
	1)
{ 
	this->client = client; 
}


_1080to480Window::~_1080to480Window()
{
}

int _1080to480Window::create_objects()
{
	int x = 10, y = 10;

	add_tool(odd_first = new _1080to480Option(client, this, 1, x, y, _("Odd field first")));
	y += 25;
	add_tool(even_first = new _1080to480Option(client, this, 0, x, y, _("Even field first")));

	show_window();
	flush();
	return 0;
}

WINDOW_CLOSE_EVENT(_1080to480Window)

int _1080to480Window::set_first_field(int first_field, int send_event)
{
	odd_first->update(first_field == 1);
	even_first->update(first_field == 0);


	if(send_event)
	{
		client->config.first_field = first_field;
		client->send_configure_change();
	}
	return 0;
}




_1080to480Option::_1080to480Option(_1080to480Main *client, 
		_1080to480Window *window, 
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

int _1080to480Option::handle_event()
{
	window->set_first_field(output, 1);
	return 1;
}



PLUGIN_THREAD_OBJECT(_1080to480Main, _1080to480Thread, _1080to480Window)







_1080to480Main::_1080to480Main(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	temp = 0;
}

_1080to480Main::~_1080to480Main()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(temp) delete temp;
}

char* _1080to480Main::plugin_title() { return N_("1080 to 480"); }
int _1080to480Main::is_realtime() { return 1; }

SHOW_GUI_MACRO(_1080to480Main, _1080to480Thread)
RAISE_WINDOW_MACRO(_1080to480Main)
SET_STRING_MACRO(_1080to480Main)
NEW_PICON_MACRO(_1080to480Main)
LOAD_CONFIGURATION_MACRO(_1080to480Main, _1080to480Config)


#define TEMP_W 854
#define TEMP_H 480
#define OUT_ROWS 240

void _1080to480Main::reduce_field(VFrame *output, VFrame *input, int src_field, int dst_field)
{
	int w = input->get_w();
	int h = input->get_h();

	if(h > output->get_h()) h = output->get_h();
	if(w > output->get_w()) h = output->get_w();

#define REDUCE_MACRO(type, temp, components) \
for(int i = 0; i < OUT_ROWS; i++) \
{ \
	int in_number1 = dst_field * 2 + src_field + (int)(i * 9 / 4) * 2; \
	int in_number2 = in_number1 + 2; \
	int in_number3 = in_number2 + 2; \
	int in_number4 = in_number3 + 2; \
	int out_number = dst_field + i * 2; \
 \
	if(in_number1 >= h) in_number1 = h - 1; \
	if(in_number2 >= h) in_number2 = h - 1; \
	if(in_number3 >= h) in_number3 = h - 1; \
	if(in_number4 >= h) in_number4 = h - 1; \
	if(out_number >= h) out_number = h - 1; \
 \
	type *in_row1 = (type*)input->get_rows()[in_number1]; \
	type *in_row2 = (type*)input->get_rows()[in_number2]; \
	type *in_row3 = (type*)input->get_rows()[in_number3]; \
	type *in_row4 = (type*)input->get_rows()[in_number4]; \
	type *out_row = (type*)output->get_rows()[out_number]; \
 \
	for(int j = 0; j < w * components; j++) \
	{ \
		*out_row++ = ((temp)*in_row1++ +  \
			(temp)*in_row2++ +  \
			(temp)*in_row3++ +  \
			(temp)*in_row4++) / 4; \
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

int _1080to480Main::process_realtime(VFrame *input, VFrame *output)
{
	load_configuration();
	if(!temp)
	{
		temp = new VFrame(0,
			input->get_w(),
			input->get_h(),
			input->get_color_model());
		temp->clear_frame();
	}

	reduce_field(temp, input, config.first_field == 0 ? 0 : 1, 0);
	reduce_field(temp, input, config.first_field == 0 ? 1 : 0, 1);
	
	output->copy_from(temp);

	return 0;
}


int _1080to480Main::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
	sprintf(directory, "%s1080to480.rc", BCASTDIR);
	
	defaults = new Defaults(directory);
	defaults->load();
	config.first_field = defaults->get("FIRST_FIELD", config.first_field);
	return 0;
}


int _1080to480Main::save_defaults()
{
	defaults->update("FIRST_FIELD", config.first_field);
	defaults->save();
	return 0;
}

void _1080to480Main::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("1080TO480");
	output.tag.set_property("FIRST_FIELD", config.first_field);
	output.append_tag();
	output.terminate_string();
}

void _1080to480Main::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	while(!input.read_tag())
	{
		if(input.tag.title_is("1080TO480"))
		{
			config.first_field = input.tag.get_property("FIRST_FIELD", config.first_field);
		}
	}
}

void _1080to480Main::update_gui()
{
	if(thread) 
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->set_first_field(config.first_field, 0);
		thread->window->unlock_window();
	}
}


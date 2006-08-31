#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "vframe.h"



#include <stdint.h>
#include <string.h>







class ShiftInterlaceWindow;
class ShiftInterlaceMain;

class ShiftInterlaceConfig
{
public:
	ShiftInterlaceConfig();

	int equivalent(ShiftInterlaceConfig &that);
	void copy_from(ShiftInterlaceConfig &that);
	void interpolate(ShiftInterlaceConfig &prev, 
		ShiftInterlaceConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);


	int odd_offset;
	int even_offset;
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

class ShiftInterlaceWindow : public BC_Window
{
public:
	ShiftInterlaceWindow(ShiftInterlaceMain *plugin, int x, int y);

	void create_objects();
	int close_event();

	ShiftInterlaceOdd *odd_offset;
	ShiftInterlaceEven *even_offset;
	ShiftInterlaceMain *plugin;
};


PLUGIN_THREAD_HEADER(ShiftInterlaceMain, ShiftInterlaceThread, ShiftInterlaceWindow)




class ShiftInterlaceMain : public PluginVClient
{
public:
	ShiftInterlaceMain(PluginServer *server);
	~ShiftInterlaceMain();

// required for all realtime plugins
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	char* plugin_title();
	VFrame* new_picon();
	int show_gui();
	void raise_window();
	void update_gui();
	int set_string();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_configuration();
	int load_defaults();
	int save_defaults();


	void shift_row(VFrame *input_frame, 
		VFrame *output_frame,
		int offset,
		int row);




	ShiftInterlaceConfig config;
	ShiftInterlaceThread *thread;
	BC_Hash *defaults;
};




PluginClient* new_plugin(PluginServer *server)
{
	return new ShiftInterlaceMain(server);
}




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
		long prev_frame, 
		long next_frame, 
		long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->odd_offset = (int)(prev.odd_offset * prev_scale + next.odd_offset * next_scale);
	this->even_offset = (int)(prev.even_offset * prev_scale + next.even_offset * next_scale);
}






ShiftInterlaceWindow::ShiftInterlaceWindow(ShiftInterlaceMain *plugin, 
	int x, 
	int y)
 : BC_Window(plugin->gui_string, 
	x,
	y,
	310, 
	100, 
	310, 
	100, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

	
void ShiftInterlaceWindow::create_objects()
{
	int x = 10, y = 10;
	int margin = 30;

	add_subwindow(new BC_Title(x, y, _("Odd offset:")));
	add_subwindow(odd_offset = new ShiftInterlaceOdd(plugin, x + 90, y));
	y += margin;
	add_subwindow(new BC_Title(x, y, _("Even offset:")));
	add_subwindow(even_offset = new ShiftInterlaceEven(plugin, x + 90, y));

	show_window();
	flush();
}

WINDOW_CLOSE_EVENT(ShiftInterlaceWindow)

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








PLUGIN_THREAD_OBJECT(ShiftInterlaceMain, ShiftInterlaceThread, ShiftInterlaceWindow)



ShiftInterlaceMain::ShiftInterlaceMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

ShiftInterlaceMain::~ShiftInterlaceMain()
{
	PLUGIN_DESTRUCTOR_MACRO
}


char* ShiftInterlaceMain::plugin_title()  { return N_("ShiftInterlace"); }
int ShiftInterlaceMain::is_realtime() { return 1; }


SHOW_GUI_MACRO(ShiftInterlaceMain, ShiftInterlaceThread)

NEW_PICON_MACRO(ShiftInterlaceMain)

SET_STRING_MACRO(ShiftInterlaceMain)

LOAD_CONFIGURATION_MACRO(ShiftInterlaceMain, ShiftInterlaceConfig)

RAISE_WINDOW_MACRO(ShiftInterlaceMain)


int ShiftInterlaceMain::load_defaults()
{
	char directory[1024], string[1024];
// set the default directory
	sprintf(directory, "%sshiftinterlace.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	config.odd_offset = defaults->get("ODD_OFFSET", config.odd_offset);
	config.even_offset = defaults->get("EVEN_OFFSET", config.even_offset);
	return 0;
}

int ShiftInterlaceMain::save_defaults()
{
	defaults->update("ODD_OFFSET", config.odd_offset);
	defaults->update("EVEN_OFFSET", config.even_offset);
	defaults->save();
	return 0;
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
	output.append_newline();
	output.terminate_string();
// data is now in *text
}

void ShiftInterlaceMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("SHIFTINTERLACE"))
			{
				config.odd_offset = input.tag.get_property("ODD_OFFSET", config.odd_offset);
				config.even_offset = input.tag.get_property("EVEN_OFFSET", config.even_offset);
			}
		}
	}
}

void ShiftInterlaceMain::update_gui()
{
	if(thread) 
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->odd_offset->update(config.odd_offset);
		thread->window->even_offset->update(config.even_offset);
		thread->window->unlock_window();
	}
}


#define SHIFT_ROW_MACRO(components, type, chroma_offset) \
{ \
	type *input_row = (type*)input_frame->get_rows()[row]; \
	type *output_row = (type*)output_frame->get_rows()[row]; \
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
	}
}

int ShiftInterlaceMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
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

	return 0;
}



#include "bcdisplayinfo.h"
#include "clip.h"
#include "defaults.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "language.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <string.h>
#include <stdint.h>


class Overlay;
class OverlayWindow;


class OverlayConfig
{
public:
	OverlayConfig();

	static char* mode_to_text(int mode);
	int mode;

	static char* direction_to_text(int direction);
	int direction;
	enum
	{
		BOTTOM_FIRST,
		TOP_FIRST
	};

	static char* output_to_text(int output_layer);
	int output_layer;
	enum
	{
		TOP,
		BOTTOM
	};
};





class OverlayMode : public BC_PopupMenu
{
public:
	OverlayMode(Overlay *plugin,
		int x, 
		int y);
	void create_objects();
	int handle_event();
	Overlay *plugin;
};

class OverlayDirection : public BC_PopupMenu
{
public:
	OverlayDirection(Overlay *plugin,
		int x, 
		int y);
	void create_objects();
	int handle_event();
	Overlay *plugin;
};

class OverlayOutput : public BC_PopupMenu
{
public:
	OverlayOutput(Overlay *plugin,
		int x, 
		int y);
	void create_objects();
	int handle_event();
	Overlay *plugin;
};


class OverlayWindow : public BC_Window
{
public:
	OverlayWindow(Overlay *plugin, int x, int y);
	~OverlayWindow();

	void create_objects();
	int close_event();

	Overlay *plugin;
	OverlayMode *mode;
	OverlayDirection *direction;
	OverlayOutput *output;
};


PLUGIN_THREAD_HEADER(Overlay, OverlayThread, OverlayWindow)



class Overlay : public PluginVClient
{
public:
	Overlay(PluginServer *server);
	~Overlay();


	PLUGIN_CLASS_MEMBERS(OverlayConfig, OverlayThread);

	int process_buffer(VFrame **frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	int is_multichannel();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();

	OverlayFrame *overlayer;
	VFrame *temp;
};












OverlayConfig::OverlayConfig()
{
	mode = TRANSFER_NORMAL;
	direction = OverlayConfig::BOTTOM_FIRST;
	output_layer = OverlayConfig::TOP;
}

char* OverlayConfig::mode_to_text(int mode)
{
	switch(mode)
	{
		case TRANSFER_NORMAL:
			return "Normal";
			break;

		case TRANSFER_REPLACE:
			return "Replace";
			break;

		case TRANSFER_ADDITION:
			return "Addition";
			break;

		case TRANSFER_SUBTRACT:
			return "Subtract";
			break;

		case TRANSFER_MULTIPLY:
			return "Multiply";
			break;

		case TRANSFER_DIVIDE:
			return "Divide";
			break;

		default:
			return "Normal";
			break;
	}
	return "";
}

char* OverlayConfig::direction_to_text(int direction)
{
	switch(direction)
	{
		case OverlayConfig::BOTTOM_FIRST: return "Bottom first";
		case OverlayConfig::TOP_FIRST:    return "Top first";
	}
	return "";
}

char* OverlayConfig::output_to_text(int output_layer)
{
	switch(output_layer)
	{
		case OverlayConfig::TOP:    return "Top";
		case OverlayConfig::BOTTOM: return "Bottom";
	}
	return "";
}









OverlayWindow::OverlayWindow(Overlay *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	300, 
	160, 
	300, 
	160, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

OverlayWindow::~OverlayWindow()
{
}

void OverlayWindow::create_objects()
{
	int x = 10, y = 10;

	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, "Mode:"));
	add_subwindow(mode = new OverlayMode(plugin, 
		x + title->get_w() + 5, 
		y));
	mode->create_objects();

	y += 30;
	add_subwindow(title = new BC_Title(x, y, "Layer order:"));
	add_subwindow(direction = new OverlayDirection(plugin, 
		x + title->get_w() + 5, 
		y));
	direction->create_objects();

	y += 30;
	add_subwindow(title = new BC_Title(x, y, "Output layer:"));
	add_subwindow(output = new OverlayOutput(plugin, 
		x + title->get_w() + 5, 
		y));
	output->create_objects();

	show_window();
	flush();
}

WINDOW_CLOSE_EVENT(OverlayWindow)





OverlayMode::OverlayMode(Overlay *plugin,
	int x, 
	int y)
 : BC_PopupMenu(x,
 	y,
	150,
	OverlayConfig::mode_to_text(plugin->config.mode),
	1)
{
	this->plugin = plugin;
}

void OverlayMode::create_objects()
{
	for(int i = 0; i < TRANSFER_TYPES; i++)
		add_item(new BC_MenuItem(OverlayConfig::mode_to_text(i)));
}

int OverlayMode::handle_event()
{
	char *text = get_text();

	for(int i = 0; i < TRANSFER_TYPES; i++)
	{
		if(!strcmp(text, OverlayConfig::mode_to_text(i)))
		{
			plugin->config.mode = i;
			break;
		}
	}

	plugin->send_configure_change();
	return 1;
}


OverlayDirection::OverlayDirection(Overlay *plugin,
	int x, 
	int y)
 : BC_PopupMenu(x,
 	y,
	150,
	OverlayConfig::direction_to_text(plugin->config.direction),
	1)
{
	this->plugin = plugin;
}

void OverlayDirection::create_objects()
{
	add_item(new BC_MenuItem(
		OverlayConfig::direction_to_text(
			OverlayConfig::TOP_FIRST)));
	add_item(new BC_MenuItem(
		OverlayConfig::direction_to_text(
			OverlayConfig::BOTTOM_FIRST)));
}

int OverlayDirection::handle_event()
{
	char *text = get_text();

	if(!strcmp(text, 
		OverlayConfig::direction_to_text(
			OverlayConfig::TOP_FIRST)))
		plugin->config.direction = OverlayConfig::TOP_FIRST;
	else
	if(!strcmp(text, 
		OverlayConfig::direction_to_text(
			OverlayConfig::BOTTOM_FIRST)))
		plugin->config.direction = OverlayConfig::BOTTOM_FIRST;

	plugin->send_configure_change();
	return 1;
}


OverlayOutput::OverlayOutput(Overlay *plugin,
	int x, 
	int y)
 : BC_PopupMenu(x,
 	y,
	100,
	OverlayConfig::output_to_text(plugin->config.output_layer),
	1)
{
	this->plugin = plugin;
}

void OverlayOutput::create_objects()
{
	add_item(new BC_MenuItem(
		OverlayConfig::output_to_text(
			OverlayConfig::TOP)));
	add_item(new BC_MenuItem(
		OverlayConfig::output_to_text(
			OverlayConfig::BOTTOM)));
}

int OverlayOutput::handle_event()
{
	char *text = get_text();

	if(!strcmp(text, 
		OverlayConfig::direction_to_text(
			OverlayConfig::TOP)))
		plugin->config.output_layer = OverlayConfig::TOP;
	else
	if(!strcmp(text, 
		OverlayConfig::direction_to_text(
			OverlayConfig::BOTTOM)))
		plugin->config.output_layer = OverlayConfig::BOTTOM;

	plugin->send_configure_change();
	return 1;
}









PLUGIN_THREAD_OBJECT(Overlay, OverlayThread, OverlayWindow)










REGISTER_PLUGIN(Overlay)






Overlay::Overlay(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	overlayer = 0;
	temp = 0;
}


Overlay::~Overlay()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(overlayer) delete overlayer;
	if(temp) delete temp;
}



int Overlay::process_buffer(VFrame **frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

	if(!temp) temp = new VFrame(0,
		frame[0]->get_w(),
		frame[0]->get_h(),
		frame[0]->get_color_model(),
		-1);

	if(!overlayer)
		overlayer = new OverlayFrame(get_project_smp() + 1);

// Inclusive layer numbers
	int input_layer1;
	int input_layer2;
	int step;
	int output_layer;
	VFrame *output;

	if(config.direction == OverlayConfig::BOTTOM_FIRST)
	{
		input_layer1 = get_total_buffers() - 1;
		input_layer2 = -1;
		step = -1;
	}
	else
	{
		input_layer1 = 0;
		input_layer2 = get_total_buffers();
		step = 1;
	}

	if(config.output_layer == OverlayConfig::TOP)
	{
		output_layer = 0;
	}
	else
	{
		output_layer = get_total_buffers() - 1;
	}



// Direct copy the first layer
	output = frame[output_layer];
	read_frame(output, 
		input_layer1, 
		start_position,
		frame_rate);
	for(int i = input_layer1 + step; i != input_layer2; i += step)
	{
		read_frame(temp, 
			i, 
			start_position,
			frame_rate);
		overlayer->overlay(output,
			temp,
			0,
			0,
			output->get_w(),
			output->get_h(),
			0,
			0,
			output->get_w(),
			output->get_h(),
			1,
			config.mode,
			NEAREST_NEIGHBOR);
	}


	return 0;
}




char* Overlay::plugin_title() { return N_("Overlay"); }
int Overlay::is_realtime() { return 1; }
int Overlay::is_multichannel() { return 1; }



NEW_PICON_MACRO(Overlay) 

SHOW_GUI_MACRO(Overlay, OverlayThread)

RAISE_WINDOW_MACRO(Overlay)

SET_STRING_MACRO(Overlay);

int Overlay::load_configuration()
{
	KeyFrame *prev_keyframe;
	prev_keyframe = get_prev_keyframe(get_source_position());
	read_data(prev_keyframe);
	return 0;
}

int Overlay::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%soverlay.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.mode = defaults->get("MODE", config.mode);
	config.direction = defaults->get("DIRECTION", config.direction);
	config.output_layer = defaults->get("OUTPUT_LAYER", config.output_layer);
	return 0;
}

int Overlay::save_defaults()
{
	defaults->update("MODE", config.mode);
	defaults->update("DIRECTION", config.direction);
	defaults->update("OUTPUT_LAYER", config.output_layer);
	defaults->save();
	return 0;
}

void Overlay::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("OVERLAY");
	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("DIRECTION", config.direction);
	output.tag.set_property("OUTPUT_LAYER", config.output_layer);
	output.append_tag();
	output.terminate_string();
}

void Overlay::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("OVERLAY"))
		{
			config.mode = input.tag.get_property("MODE", config.mode);
			config.direction = input.tag.get_property("DIRECTION", config.direction);
			config.output_layer = input.tag.get_property("OUTPUT_LAYER", config.output_layer);
		}
	}
}

void Overlay::update_gui()
{
	if(thread)
	{
		thread->window->lock_window("Overlay::update_gui");
		thread->window->mode->set_text(OverlayConfig::mode_to_text(config.mode));
		thread->window->direction->set_text(OverlayConfig::direction_to_text(config.direction));
		thread->window->output->set_text(OverlayConfig::output_to_text(config.output_layer));
		thread->window->unlock_window();
	}
}






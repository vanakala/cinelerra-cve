#include "bcdisplayinfo.h"
#include "defaults.h"
#include "filexml.h"
#include "freezeframe.h"
#include "picon_png.h"



#include <string.h>

REGISTER_PLUGIN(FreezeFrameMain)





FreezeFrameConfig::FreezeFrameConfig()
{
	enabled = 0;
}

void FreezeFrameConfig::copy_from(FreezeFrameConfig &that)
{
	enabled = that.enabled;
}

int FreezeFrameConfig::equivalent(FreezeFrameConfig &that)
{
	return enabled == that.enabled;
}

void FreezeFrameConfig::interpolate(FreezeFrameConfig &prev, 
	FreezeFrameConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	this->enabled = prev.enabled;
}










FreezeFrameWindow::FreezeFrameWindow(FreezeFrameMain *client, int x, int y)
 : BC_Window(client->get_gui_string(),
 	x,
	y,
	100,
	100,
	100,
	100,
	0,
	0,
	1)
{
	this->client = client; 
}

FreezeFrameWindow::~FreezeFrameWindow()
{
}

int FreezeFrameWindow::create_objects()
{
	int x = 10, y = 10;
	add_tool(enabled = new FreezeFrameToggle(client, 
		x, 
		y));
	show_window();
	flush();
	return 0;
}

int FreezeFrameWindow::close_event()
{
	set_done(1);
	return 1;
}




PLUGIN_THREAD_OBJECT(FreezeFrameMain, FreezeFrameThread, FreezeFrameWindow)





FreezeFrameToggle::FreezeFrameToggle(FreezeFrameMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.enabled, "Enabled")
{
	this->client = client;
}
FreezeFrameToggle::~FreezeFrameToggle()
{
}
int FreezeFrameToggle::handle_event()
{
	client->config.enabled = get_value();
	client->send_configure_change();
	return 1;
}












FreezeFrameMain::FreezeFrameMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	first_frame = 0;
}

FreezeFrameMain::~FreezeFrameMain()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(first_frame) delete first_frame;
}

char* FreezeFrameMain::plugin_title() { return "Freeze Frame"; }

int FreezeFrameMain::is_synthesis()
{
	return 1;
}

int FreezeFrameMain::is_realtime() { return 1; }

SHOW_GUI_MACRO(FreezeFrameMain, FreezeFrameThread)

RAISE_WINDOW_MACRO(FreezeFrameMain)

SET_STRING_MACRO(FreezeFrameMain)

NEW_PICON_MACRO(FreezeFrameMain)

LOAD_CONFIGURATION_MACRO(FreezeFrameMain, FreezeFrameConfig)

void FreezeFrameMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->enabled->update(config.enabled);
		thread->window->unlock_window();
	}
}

void FreezeFrameMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("FREEZEFRAME");
	output.append_tag();
	if(config.enabled)
	{
		output.tag.set_title("ENABLED");
		output.append_tag();
	}
	output.terminate_string();
// data is now in *text
}

void FreezeFrameMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;
	config.enabled = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("ENABLED"))
			{
				config.enabled = 1;
			}
		}
	}
}

int FreezeFrameMain::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sfreezeframe.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.enabled = defaults->get("ENABLED", config.enabled);
	return 0;
}

int FreezeFrameMain::save_defaults()
{
	defaults->update("ENABLED", config.enabled);
	defaults->save();
	return 0;
}






int FreezeFrameMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	load_configuration();
	KeyFrame *prev_keyframe;
	int new_keyframe;
	prev_keyframe = get_prev_keyframe(get_source_position());
	new_keyframe = (prev_keyframe->position == get_source_position());

	if(!first_frame && config.enabled || new_keyframe)
	{
		if(!first_frame)
			first_frame = new VFrame(0, 
				input_ptr->get_w(), 
				input_ptr->get_h(),
				input_ptr->get_color_model());
		first_frame->copy_from(input_ptr);
		output_ptr->copy_from(input_ptr);
	}
	else
	if(!first_frame && !config.enabled)
	{
		output_ptr->copy_from(input_ptr);
	}
	else
	if(first_frame && !config.enabled)
	{
		delete first_frame;
		first_frame = 0;
		output_ptr->copy_from(input_ptr);
	}
	else
	if(first_frame && config.enabled)
	{
		output_ptr->copy_from(first_frame);
	}
	return 0;
}


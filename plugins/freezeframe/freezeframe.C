#include "bcdisplayinfo.h"
#include "defaults.h"
#include "filexml.h"
#include "freezeframe.h"
#include "language.h"
#include "picon_png.h"



#include <string.h>


REGISTER_PLUGIN(FreezeFrameMain)





FreezeFrameConfig::FreezeFrameConfig()
{
	enabled = 0;
	line_double = 0;
}

void FreezeFrameConfig::copy_from(FreezeFrameConfig &that)
{
	enabled = that.enabled;
	line_double = that.line_double;
}

int FreezeFrameConfig::equivalent(FreezeFrameConfig &that)
{
	return enabled == that.enabled &&
		line_double == that.line_double;
}

void FreezeFrameConfig::interpolate(FreezeFrameConfig &prev, 
	FreezeFrameConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	this->enabled = prev.enabled;
	this->line_double = prev.line_double;
}










FreezeFrameWindow::FreezeFrameWindow(FreezeFrameMain *client, int x, int y)
 : BC_Window(client->get_gui_string(),
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

FreezeFrameWindow::~FreezeFrameWindow()
{
}

int FreezeFrameWindow::create_objects()
{
	int x = 10, y = 10;
	add_tool(enabled = new FreezeFrameToggle(client, 
		&client->config.enabled,
		x, 
		y,
		_("Enabled")));
	y += 30;
	add_tool(line_double = new FreezeFrameToggle(client, 
		&client->config.line_double,
		x, 
		y,
		_("Line double")));
	show_window();
	flush();
	return 0;
}

WINDOW_CLOSE_EVENT(FreezeFrameWindow)




PLUGIN_THREAD_OBJECT(FreezeFrameMain, FreezeFrameThread, FreezeFrameWindow)





FreezeFrameToggle::FreezeFrameToggle(FreezeFrameMain *client, 
	int *value, 
	int x, 
	int y,
	char *text)
 : BC_CheckBox(x, y, *value, text)
{
	this->client = client;
	this->value = value;
}
FreezeFrameToggle::~FreezeFrameToggle()
{
}
int FreezeFrameToggle::handle_event()
{
	*value = get_value();
	client->send_configure_change();
	return 1;
}












FreezeFrameMain::FreezeFrameMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	first_frame = 0;
	first_frame_position = -1;
}

FreezeFrameMain::~FreezeFrameMain()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(first_frame) delete first_frame;
}

char* FreezeFrameMain::plugin_title() { return N_("Freeze Frame"); }
int FreezeFrameMain::is_synthesis() { return 1; }
int FreezeFrameMain::is_realtime() { return 1; }


SHOW_GUI_MACRO(FreezeFrameMain, FreezeFrameThread)

RAISE_WINDOW_MACRO(FreezeFrameMain)

SET_STRING_MACRO(FreezeFrameMain)

NEW_PICON_MACRO(FreezeFrameMain)

int FreezeFrameMain::load_configuration()
{
	KeyFrame *prev_keyframe = get_prev_keyframe(get_source_position());
	int64_t prev_position = edl_to_local(prev_keyframe->position);
	if(prev_position < get_source_start()) prev_position = get_source_start();
	read_data(prev_keyframe);
// Invalidate stored frame
	if(config.enabled) first_frame_position = prev_position;
	return 0;
}

void FreezeFrameMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->enabled->update(config.enabled);
		thread->window->line_double->update(config.line_double);
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
	if(config.line_double)
	{
		output.tag.set_title("LINE_DOUBLE");
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
	config.line_double = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("ENABLED"))
			{
				config.enabled = 1;
			}
			if(input.tag.title_is("LINE_DOUBLE"))
			{
				config.line_double = 1;
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
	config.line_double = defaults->get("LINE_DOUBLE", config.line_double);
	return 0;
}

int FreezeFrameMain::save_defaults()
{
	defaults->update("ENABLED", config.enabled);
	defaults->update("LINE_DOUBLE", config.line_double);
	defaults->save();
	return 0;
}






int FreezeFrameMain::process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate)
{
	int64_t previous_first_frame = first_frame_position;
	load_configuration();


// Just entered frozen range
	if(!first_frame && config.enabled)
	{
		if(!first_frame)
			first_frame = new VFrame(0, 
				frame->get_w(), 
				frame->get_h(),
				frame->get_color_model());
		read_frame(first_frame, 
				0, 
				first_frame_position,
				frame_rate);
		frame->copy_from(first_frame);
	}
	else
// Still not frozen
	if(!first_frame && !config.enabled)
	{
		read_frame(frame, 
			0, 
			start_position,
			frame_rate);
	}
	else
// Just left frozen range
	if(first_frame && !config.enabled)
	{
		delete first_frame;
		first_frame = 0;
		read_frame(frame, 
			0, 
			start_position,
			frame_rate);
	}
	else
// Still frozen
	if(first_frame && config.enabled)
	{
// Had a keyframe in frozen range.  Load new first frame
		if(previous_first_frame != first_frame_position)
		{
			read_frame(first_frame, 
				0, 
				first_frame_position,
				frame_rate);
		}
		frame->copy_from(first_frame);
	}


// Line double to support interlacing
	if(config.line_double && config.enabled)
	{
		for(int i = 0; i < frame->get_h() - 1; i += 2)
		{
			memcpy(frame->get_rows()[i + 1], 
				frame->get_rows()[i], 
				frame->get_bytes_per_line());
		}
	}



	return 0;
}


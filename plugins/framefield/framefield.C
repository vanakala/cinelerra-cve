#include "bcdisplayinfo.h"
#include "defaults.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <string.h>

#define TOP_FIELD_FIRST 0
#define BOTTOM_FIELD_FIRST 1

class FrameField;
class FrameFieldWindow;


class FrameFieldConfig
{
public:
	FrameFieldConfig();
	int field_dominance;
	int double_lines;
};




class FrameFieldTop : public BC_Radial
{
public:
	FrameFieldTop(FrameField *plugin, FrameFieldWindow *gui, int x, int y);
	int handle_event();
	FrameField *plugin;
	FrameFieldWindow *gui;
};


class FrameFieldBottom : public BC_Radial
{
public:
	FrameFieldBottom(FrameField *plugin, FrameFieldWindow *gui, int x, int y);
	int handle_event();
	FrameField *plugin;
	FrameFieldWindow *gui;
};


class FrameFieldDouble : public BC_CheckBox
{
public:
	FrameFieldDouble(FrameField *plugin, FrameFieldWindow *gui, int x, int y);
	int handle_event();
	FrameField *plugin;
	FrameFieldWindow *gui;
};

class FrameFieldWindow : public BC_Window
{
public:
	FrameFieldWindow(FrameField *plugin, int x, int y);
	void create_objects();
	int close_event();
	FrameField *plugin;
	FrameFieldTop *top;
	FrameFieldBottom *bottom;
	FrameFieldDouble *double_lines;
};


PLUGIN_THREAD_HEADER(FrameField, FrameFieldThread, FrameFieldWindow)



class FrameField : public PluginVClient
{
public:
	FrameField(PluginServer *server);
	~FrameField();

	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	char* plugin_title();
	VFrame* new_picon();
	int show_gui();
	void load_configuration();
	int set_string();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void raise_window();
	void update_gui();

	int current_frame;
	VFrame *prev_frame;
	FrameFieldThread *thread;
	FrameFieldConfig config;
	Defaults *defaults;
};












FrameFieldConfig::FrameFieldConfig()
{
	field_dominance = TOP_FIELD_FIRST;
	double_lines = 1;
}









FrameFieldWindow::FrameFieldWindow(FrameField *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	210, 
	160, 
	200, 
	160, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

void FrameFieldWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(top = new FrameFieldTop(plugin, this, x, y));
	y += 30;
	add_subwindow(bottom = new FrameFieldBottom(plugin, this, x, y));
	y += 30;
	add_subwindow(double_lines = new FrameFieldDouble(plugin, this, x, y));
	show_window();
	flush();
}

int FrameFieldWindow::close_event()
{
	set_done(1);
	return 1;
}












FrameFieldTop::FrameFieldTop(FrameField *plugin, 
	FrameFieldWindow *gui, 
	int x, 
	int y)
 : BC_Radial(x, 
	y, 
	plugin->config.field_dominance == TOP_FIELD_FIRST,
	"Top field first")
{
	this->plugin = plugin;
	this->gui = gui;
}

int FrameFieldTop::handle_event()
{
	plugin->config.field_dominance = TOP_FIELD_FIRST;
	gui->bottom->update(0);
	plugin->send_configure_change();
	return 1;
}





FrameFieldBottom::FrameFieldBottom(FrameField *plugin, 
	FrameFieldWindow *gui, 
	int x, 
	int y)
 : BC_Radial(x, 
	y, 
	plugin->config.field_dominance == BOTTOM_FIELD_FIRST,
	"Bottom field first")
{
	this->plugin = plugin;
	this->gui = gui;
}

int FrameFieldBottom::handle_event()
{
	plugin->config.field_dominance = BOTTOM_FIELD_FIRST;
	gui->top->update(0);
	plugin->send_configure_change();
	return 1;
}



FrameFieldDouble::FrameFieldDouble(FrameField *plugin, 
	FrameFieldWindow *gui, 
	int x, 
	int y)
 : BC_CheckBox(x, 
	y, 
	plugin->config.double_lines,
	"Double lines")
{
	this->plugin = plugin;
	this->gui = gui;
}

int FrameFieldDouble::handle_event()
{
	plugin->config.double_lines = get_value();
	plugin->send_configure_change();
	return 1;
}



PLUGIN_THREAD_OBJECT(FrameField, FrameFieldThread, FrameFieldWindow)










REGISTER_PLUGIN(FrameField)






FrameField::FrameField(PluginServer *server)
 : PluginVClient(server)
{
	prev_frame = 0;
	thread = 0;
	current_frame = 0;
	load_defaults();
}


FrameField::~FrameField()
{
	if(thread)
	{
		thread->window->set_done(0);
		thread->completion.lock();
		delete thread;
	}

	save_defaults();
	delete defaults;

	if(prev_frame) delete prev_frame;
}


// 0 - current frame field 0, prev frame field 1
// 1 - current frame field 0, current frame field 1, copy current to prev
// 2 - current frame field 0, prev frame field 1

int FrameField::process_realtime(VFrame *input, VFrame *output)
{
	load_configuration();
	
	int row_size = VFrame::calculate_bytes_per_pixel(input->get_color_model()) * input->get_w();
	int start_row;

	if(!prev_frame)
	{
		prev_frame = new VFrame(0, 
			input->get_w(), 
			input->get_h(), 
			input->get_color_model());
	}

	unsigned char **current_rows = input->get_rows();
	unsigned char **prev_rows = prev_frame->get_rows();
	unsigned char **output_rows = output->get_rows();

	if(current_frame == 0)
	{
//printf("FrameField::process_realtime 1 %d\n", config.field_dominance);
		if(config.field_dominance == TOP_FIELD_FIRST) 
		{
			for(int i = 0; i < input->get_h(); i += 2)
			{
				if(config.double_lines)
				{
					memcpy(output_rows[i], current_rows[i], row_size);
					memcpy(output_rows[i + 1], current_rows[i], row_size);
				}
				else
				{
					memcpy(output_rows[i], current_rows[i], row_size);
					memcpy(output_rows[i + 1], prev_rows[i + 1], row_size);
				}
			}
		}
		else
		{
			for(int i = 0; i < input->get_h(); i += 2)
			{
				if(config.double_lines)
				{
					memcpy(output_rows[i], current_rows[i + 1], row_size);
					memcpy(output_rows[i + 1], current_rows[i + 1], row_size);
				}
				else
				{
					memcpy(output_rows[i], prev_rows[i], row_size);
					memcpy(output_rows[i + 1], current_rows[i + 1], row_size);
				}
			}
		}
	}
	else
	{
		if(config.double_lines)
		{
			if(config.field_dominance == TOP_FIELD_FIRST) 
			{
				for(int i = 0; i < input->get_h(); i += 2)
				{
					if(config.double_lines)
					{
						memcpy(output_rows[i], current_rows[i + 1], row_size);
						memcpy(output_rows[i + 1], current_rows[i + 1], row_size);
					}
				}
			}
			else
			{
				for(int i = 0; i < input->get_h(); i += 2)
				{
					if(config.double_lines)
					{
						memcpy(output_rows[i], current_rows[i], row_size);
						memcpy(output_rows[i + 1], current_rows[i], row_size);
					}
				}
			}
		}
		else
		{
			prev_frame->copy_from(input);
			output->copy_from(input);
		}
	}




	current_frame = !current_frame;
}

int FrameField::is_realtime()
{
	return 1;
}

char* FrameField::plugin_title()
{
	return "Frames to fields";
}

NEW_PICON_MACRO(FrameField) 

SHOW_GUI_MACRO(FrameField, FrameFieldThread)

RAISE_WINDOW_MACRO(FrameField)

SET_STRING_MACRO(FrameField);

void FrameField::load_configuration()
{
	KeyFrame *prev_keyframe;
	prev_keyframe = get_prev_keyframe(get_source_position());
	read_data(prev_keyframe);
}

int FrameField::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sframefield.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.field_dominance = defaults->get("DOMINANCE", config.field_dominance);
	config.double_lines = defaults->get("DOUBLE", config.double_lines);
	return 0;
}

int FrameField::save_defaults()
{
	defaults->update("DOMINANCE", config.field_dominance);
	defaults->update("DOUBLE", config.double_lines);
	defaults->save();
	return 0;
}

void FrameField::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("FRAME_FIELD");
	output.tag.set_property("DOMINANCE", config.field_dominance);
	output.tag.set_property("DOUBLE", config.double_lines);
	output.append_tag();
	output.terminate_string();
}

void FrameField::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("FRAME_FIELD"))
		{
			config.field_dominance = input.tag.get_property("DOMINANCE", config.field_dominance);
			config.double_lines = input.tag.get_property("DOUBLE", config.double_lines);
		}
	}
}

void FrameField::update_gui()
{
	if(thread)
	{
		thread->window->lock_window();
		thread->window->top->update(config.field_dominance == TOP_FIELD_FIRST);
		thread->window->bottom->update(config.field_dominance == BOTTOM_FIELD_FIRST);
		thread->window->double_lines->update(config.double_lines);
		thread->window->unlock_window();
	}
}






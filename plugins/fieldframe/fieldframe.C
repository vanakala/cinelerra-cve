#include "bcdisplayinfo.h"
#include "defaults.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "mainprogress.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <string.h>

#define TOP_FIELD_FIRST 0
#define BOTTOM_FIELD_FIRST 1

class FieldFrame;
class FieldFrameWindow;


class FieldFrameConfig
{
public:
	FieldFrameConfig();
	int field_dominance;
};




class FieldFrameTop : public BC_Radial
{
public:
	FieldFrameTop(FieldFrame *plugin, FieldFrameWindow *gui, int x, int y);
	int handle_event();
	FieldFrame *plugin;
	FieldFrameWindow *gui;
};


class FieldFrameBottom : public BC_Radial
{
public:
	FieldFrameBottom(FieldFrame *plugin, FieldFrameWindow *gui, int x, int y);
	int handle_event();
	FieldFrame *plugin;
	FieldFrameWindow *gui;
};

class FieldFrameAvg : public BC_CheckBox
{
public:
	FieldFrameAvg(FieldFrame *plugin, FieldFrameWindow *gui, int x, int y);
	int handle_event();
	FieldFrame *plugin;
	FieldFrameWindow *gui;
};

class FieldFrameWindow : public BC_Window
{
public:
	FieldFrameWindow(FieldFrame *plugin, int x, int y);
	void create_objects();
	int close_event();
	FieldFrame *plugin;
	FieldFrameTop *top;
	FieldFrameBottom *bottom;
};




class FieldFrame : public PluginVClient
{
public:
	FieldFrame(PluginServer *server);
	~FieldFrame();

	char* plugin_title();
	VFrame* new_picon();
	int get_parameters();
	int load_defaults();
	int save_defaults();
	int start_loop();
	int stop_loop();
	int process_loop(VFrame *output);
	double get_framerate();

	int current_field;
	long input_position;
	VFrame *prev_frame, *next_frame;
	FieldFrameConfig config;
	Defaults *defaults;
	MainProgressBar *progress;
};












FieldFrameConfig::FieldFrameConfig()
{
	field_dominance = TOP_FIELD_FIRST;
}









FieldFrameWindow::FieldFrameWindow(FieldFrame *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	230, 
	160, 
	230, 
	160, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

void FieldFrameWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(top = new FieldFrameTop(plugin, this, x, y));
	y += 30;
	add_subwindow(bottom = new FieldFrameBottom(plugin, this, x, y));

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));

	show_window();
	flush();
}

int FieldFrameWindow::close_event()
{
	set_done(1);
	return 1;
}












FieldFrameTop::FieldFrameTop(FieldFrame *plugin, 
	FieldFrameWindow *gui, 
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

int FieldFrameTop::handle_event()
{
	plugin->config.field_dominance = TOP_FIELD_FIRST;
	gui->bottom->update(0);
	return 1;
}





FieldFrameBottom::FieldFrameBottom(FieldFrame *plugin, 
	FieldFrameWindow *gui, 
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

int FieldFrameBottom::handle_event()
{
	plugin->config.field_dominance = BOTTOM_FIELD_FIRST;
	gui->top->update(0);
	return 1;
}




















REGISTER_PLUGIN(FieldFrame)


FieldFrame::FieldFrame(PluginServer *server)
 : PluginVClient(server)
{
	prev_frame = 0;
	next_frame = 0;
	current_field = 0;
	load_defaults();
}


FieldFrame::~FieldFrame()
{
	save_defaults();
	delete defaults;

	if(prev_frame) delete prev_frame;
	if(next_frame) delete next_frame;
}

char* FieldFrame::plugin_title()
{
	return "Fields to frames";
}

NEW_PICON_MACRO(FieldFrame) 

double FieldFrame::get_framerate()
{
	return project_frame_rate / 2;
}

int FieldFrame::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sfieldframe.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.field_dominance = defaults->get("DOMINANCE", config.field_dominance);
	return 0;
}

int FieldFrame::save_defaults()
{
	defaults->update("DOMINANCE", config.field_dominance);
	defaults->save();
	return 0;
}

int FieldFrame::get_parameters()
{
	BC_DisplayInfo info;
	FieldFrameWindow window(this, info.get_abs_cursor_x(), info.get_abs_cursor_y());
	window.create_objects();
	int result = window.run_window();
	
	return result;
}

int FieldFrame::start_loop()
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

int FieldFrame::stop_loop()
{
	if(PluginClient::interactive)
	{
		progress->stop_progress();
		delete progress;
	}
	return 0;
}

int FieldFrame::process_loop(VFrame *output)
{
	int result = 0;

	if(!prev_frame)
	{
		prev_frame = new VFrame(0, output->get_w(), output->get_h(), output->get_color_model());
	}
	if(!next_frame)
	{
		next_frame = new VFrame(0, output->get_w(), output->get_h(), output->get_color_model());
	}

	read_frame(prev_frame, input_position);
	input_position++;
	read_frame(next_frame, input_position);
	input_position++;

	unsigned char **input_rows1;
	unsigned char **input_rows2;
	if(config.field_dominance == TOP_FIELD_FIRST)
	{
		input_rows1 = prev_frame->get_rows();
		input_rows2 = next_frame->get_rows();
	}
	else
	{
		input_rows1 = next_frame->get_rows();
		input_rows2 = prev_frame->get_rows();
	}

	unsigned char **output_rows = output->get_rows();
	int row_size = VFrame::calculate_bytes_per_pixel(output->get_color_model()) * output->get_w();
	for(int i = 0; i < output->get_h() - 1; i += 2)
	{
		memcpy(output_rows[i], input_rows1[i], row_size);
		memcpy(output_rows[i + 1], input_rows2[i + 1], row_size);
	}


	if(PluginClient::interactive) 
		result = progress->update(input_position - PluginClient::start);

	if(input_position >= PluginClient::end) result = 1;

	return result;
}



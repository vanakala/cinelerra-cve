#include "bcdisplayinfo.h"
#include "defaults.h"
#include "mainprogress.h"
#include "picon_png.h"
#include "reframe.h"





REGISTER_PLUGIN(ReFrame)










ReFrame::ReFrame(PluginServer *server)
 : PluginVClient(server)
{
	load_defaults();
	current_position = 0;
}

ReFrame::~ReFrame()
{
	save_defaults();
	delete defaults;
}

char* ReFrame::plugin_title() { return "Reframe"; }

NEW_PICON_MACRO(ReFrame) 


int ReFrame::load_defaults()
{
	char directory[1024];

// set the default directory
	sprintf(directory, "%sreframe.rc", BCASTDIR);

// load the defaults

	defaults = new Defaults(directory);

	defaults->load();

	scale = defaults->get("SCALE", (double)1);
	return 0;
}

int ReFrame::save_defaults()
{
	defaults->update("SCALE", scale);
	defaults->save();
	return 0;
}

int ReFrame::get_parameters()
{
	BC_DisplayInfo info;
	ReFrameWindow window(this, info.get_abs_cursor_x(), info.get_abs_cursor_y());
	window.create_objects();
	int result = window.run_window();
	
	return result;
}


int ReFrame::start_loop()
{
	if(PluginClient::interactive)
	{
		char string[BCTEXTLEN];
		sprintf(string, "%s...", plugin_title());
		progress = start_progress(string, 
			(PluginClient::end - PluginClient::start));
	}

	current_position = 0;
	return 0;
}

int ReFrame::stop_loop()
{
	if(PluginClient::interactive)
	{
		progress->stop_progress();
		delete progress;
	}
	return 0;
}


int ReFrame::process_loop(VFrame *buffer)
{
	int result = 0;
	
	long input_offset = Units::to_long((double)current_position * 
		scale + 
		PluginClient::start);

	read_frame(buffer, input_offset);

	current_position++;
	input_offset = Units::to_long((double)current_position * 
		scale + 
		PluginClient::start);

	if(PluginClient::interactive) 
		result = progress->update(input_offset - PluginClient::start);

	if(input_offset >= PluginClient::end) result = 1;

	return result;
}












ReFrameOutput::ReFrameOutput(ReFrame *plugin, int x, int y)
 : BC_TextBox(x, y, 150, 1, (float)plugin->scale)
{
	this->plugin = plugin;
}

int ReFrameOutput::handle_event()
{
	plugin->scale = atof(get_text());
	return 1;
}



ReFrameWindow::ReFrameWindow(ReFrame *plugin, int x, int y)
 : BC_Window(plugin->plugin_title(), 
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

ReFrameWindow::~ReFrameWindow()
{
}


void ReFrameWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, "Scale factor:"));
	y += 20;
	add_subwindow(new ReFrameOutput(plugin, x, y));
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));

	show_window();
	flush();
}

int ReFrameWindow::close_event()
{
	set_done(1);
	return 1;
}





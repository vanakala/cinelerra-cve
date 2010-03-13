
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

#include "bcdisplayinfo.h"
#include "bchash.h"
#include "language.h"
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

const char* ReFrame::plugin_title() { return N_("Reframe"); }

NEW_PICON_MACRO(ReFrame) 


int ReFrame::load_defaults()
{
	char directory[1024];

// set the default directory
	sprintf(directory, "%sreframe.rc", BCASTDIR);

// load the defaults

	defaults = new BC_Hash(directory);

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
	
	int64_t input_offset = Units::to_int64((double)current_position * 
		scale + 
		PluginClient::start);

	read_frame(buffer, input_offset);

	current_position++;
	input_offset = Units::to_int64((double)current_position * 
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
	add_subwindow(new BC_Title(x, y, _("Scale factor:")));
	y += 20;
	add_subwindow(new ReFrameOutput(plugin, x, y));
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));

	show_window();
	flush();
}

WINDOW_CLOSE_EVENT(ReFrameWindow)



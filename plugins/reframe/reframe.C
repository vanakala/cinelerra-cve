
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

#include "bchash.h"
#include "language.h"
#include "mainprogress.h"
#include "picon_png.h"
#include "reframe.h"


REGISTER_PLUGIN


ReFrame::ReFrame(PluginServer *server)
 : PluginVClient(server)
{
	current_pts = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

ReFrame::~ReFrame()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void ReFrame::load_defaults()
{
	defaults = load_defaults_file("reframe.rc");

	scale = defaults->get("SCALE", (double)1);
}

void ReFrame::save_defaults()
{
	defaults->update("SCALE", scale);
	defaults->save();
}

void ReFrame::start_loop()
{
	if(PluginClient::interactive)
	{
		char string[BCTEXTLEN];
		sprintf(string, "%s...", plugin_title());
		progress = start_progress(string, 
			(PluginClient::end_pts - PluginClient::start_pts) * 100);
	}
	current_pts = 0;
}

void ReFrame::stop_loop()
{
	if(PluginClient::interactive)
	{
		progress->stop_progress();
		delete progress;
	}
}

int ReFrame::process_loop(VFrame *buffer)
{
	int result = 0;

	ptstime input_pts = current_pts * scale + PluginClient::start_pts;

	buffer->set_pts(input_pts);
	get_frame(buffer);
	buffer->set_pts(current_pts);
	current_pts = buffer->next_pts();

	if(PluginClient::interactive) 
		result = progress->update((input_pts - PluginClient::start_pts) * 100);

	if(input_pts >= PluginClient::end_pts) result = 1;

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
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Scale factor:")));
	y += 20;
	add_subwindow(new ReFrameOutput(plugin, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

ReFrameWindow::~ReFrameWindow()
{
}

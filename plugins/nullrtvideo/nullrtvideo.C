
/*
 * CINELERRA
 * Copyright (C) 2012 Einar Rünkaru <einarry at smail dot ee>
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
#include "bctitle.h"
#include "filexml.h"
#include "picon_png.h"
#include "vframe.h"
#include "nullrtvideo.h"

#include <stdint.h>
#include <string.h>

// Registers plugin
REGISTER_PLUGIN

NRTVideoConfig::NRTVideoConfig()
{
	onoff = 0;
}

NRTVideoSwitch::NRTVideoSwitch(NRTVideo *plugin, 
	int x,
	int y)
 : BC_Radial(x,
		y,
		plugin->config.onoff,
		_("Clear"))
{
	this->plugin = plugin;
}

int NRTVideoSwitch::handle_event()
{
	plugin->config.onoff = plugin->config.onoff ? 0 : 1;
	plugin->send_configure_change();
	return 0;
}

/*
 * The gui of the plugin
 * Using the constuctor macro is mandatory
 * The constructor macro must be near the end of constructor after
 * creating the elements of gui.
 */
NRTVideoWindow::NRTVideoWindow(NRTVideo *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y,
	200,
	50)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Status:")));
	add_subwindow(toggle = new NRTVideoSwitch(plugin,
		x + 60,
		y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void NRTVideoWindow::update()
{
	toggle->update(plugin->config.onoff);
}

/*
 * All methods of the plugin thread class
 */
PLUGIN_THREAD_METHODS

/*
 * The processor of the plugin
 * Using the constructor and the destructor macro is mandatory
 */
NRTVideo::NRTVideo(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

NRTVideo::~NRTVideo()
{
	PLUGIN_DESTRUCTOR_MACRO
}

/*
 * Creates most of the api methods of the plugin
 */
PLUGIN_CLASS_METHODS

void NRTVideo::load_defaults()
{
// Loads defaults from the named rc-file
	defaults = load_defaults_file("nullrtvideo.rc");

	config.onoff = defaults->get("ONOFF", config.onoff);
}

void NRTVideo::save_defaults()
{
	defaults->update("ONOFF", config.onoff);
	defaults->save();
}

/*
 * Saves the values of the parameters to the project
 */
void NRTVideo::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("NRTVIDEO");
	output.tag.set_property("ONOFF", config.onoff);
	output.append_tag();
	output.tag.set_title("/NRTVIDEO");
	output.append_tag();
	keyframe->set_data(output.string);
}

void NRTVideo::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("NRTVIDEO"))
		{
			config.onoff = input.tag.get_property("ONOFF", config.onoff);
		}
	}
}

/*
 * Loads parameters from keyframe
 */
int NRTVideo::load_configuration()
{
	int prev_val = config.onoff;
	read_data(prev_keyframe_pts(source_pts));
	return !(prev_val == config.onoff);
}

/*
 * Pull frame and apply modifications
 * It is possible to use here
 * void process_realtime(VFrame *input, VFrame *output)
 *    - here input is already filled
 * multichannel plugin gets here arrays of pointers
 * prototype: void process_frame(VFrame **frame)
 *    - number of channels is total_in_buffers
 */
void NRTVideo::process_frame(VFrame *frame)
{
	load_configuration();

	get_frame(frame);

	if(config.onoff)
		frame->clear_frame();
}

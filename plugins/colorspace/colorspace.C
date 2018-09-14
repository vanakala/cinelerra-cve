
/*
 * CINELERRA
 * Copyright (C) 2018 Einar Rünkaru <einarrunkaru at gmail dot com>
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
#include "colorspace.h"

#include <stdint.h>
#include <string.h>

// Registers plugin
REGISTER_PLUGIN

ColorSpaceConfig::ColorSpaceConfig()
{
	onoff = 0;
}

ColorSpaceSwitch::ColorSpaceSwitch(ColorSpace *plugin,
	int x,
	int y)
 : BC_Radial(x,
		y,
		plugin->config.onoff,
		_("Clear"))
{
	this->plugin = plugin;
}

int ColorSpaceSwitch::handle_event()
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
ColorSpaceWindow::ColorSpaceWindow(ColorSpace *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y,
	200,
	50)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Status:")));
	add_subwindow(toggle = new ColorSpaceSwitch(plugin,
		x + 60,
		y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void ColorSpaceWindow::update()
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
ColorSpace::ColorSpace(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

ColorSpace::~ColorSpace()
{
	PLUGIN_DESTRUCTOR_MACRO
}

/*
 * Creates most of the api methods of the plugin
 */
PLUGIN_CLASS_METHODS

void ColorSpace::load_defaults()
{
// Loads defaults from the named rc-file
	defaults = load_defaults_file("colorspace.rc");

	config.onoff = defaults->get("ONOFF", config.onoff);
}

void ColorSpace::save_defaults()
{
	defaults->update("ONOFF", config.onoff);
	defaults->save();
}

/*
 * Saves the values of the parameters to the project
 */
void ColorSpace::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("COLORSPACE");
	output.tag.set_property("ONOFF", config.onoff);
	output.append_tag();
	output.tag.set_title("/COLORSPACE");
	output.append_tag();
	output.terminate_string();
}

void ColorSpace::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	while(!input.read_tag())
	{
		if(input.tag.title_is("COLORSPACE"))
		{
			config.onoff = input.tag.get_property("ONOFF", config.onoff);
		}
	}
}

/*
 * Loads parameters from keyframe
 */
int ColorSpace::load_configuration()
{
	int prev_val = config.onoff;
	read_data(prev_keyframe_pts(source_pts));
	return !(prev_val == config.onoff);
}

/*
 * Pull frame and apply modifications
 */
void ColorSpace::process_frame(VFrame *frame)
{
	load_configuration();

	get_frame(frame);

	if(config.onoff)
		frame->clear_frame();
}

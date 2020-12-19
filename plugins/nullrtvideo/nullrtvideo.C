// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2012 Einar Rünkaru <einarrunkaru@gmail dot com>

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
	return 1;
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
 * Optional entry called when plugin is idle
 * Plugin should release excessive resources
 *  temporary frames, working byffers, etc
 */
void NRTVideo::reset_plugin()
{
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

	if(!keyframe)
		return;

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
	read_data(get_prev_keyframe(source_pts));
	return !(prev_val == config.onoff);
}

/*
 * Apply modifications to frame
 * Multichannel plugin gets here arrays of pointers
 * prototype: void process_tmpframes(VFrame **frame)
 *    - number of channels is total_in_buffers
 * in new frame is needed, it must be aquired with clone_vframe
 * input frame can be freed with release_vframe
 * temporary frames should be aquired with clone_vframe and
 *   released with release_vframe asap
 * When parameters change load_configuration returns NZ,
 *  update_gui then
 */
VFrame *NRTVideo::process_tmpframe(VFrame *frame)
{
	if(load_configuration())
		update_gui();

	if(config.onoff)
		frame->clear_frame();
	return frame;
}

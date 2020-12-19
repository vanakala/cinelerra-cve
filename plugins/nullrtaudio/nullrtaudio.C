// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2012 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "aframe.h"
#include "bchash.h"
#include "bctitle.h"
#include "filexml.h"
#include "picon_png.h"
#include "nullrtaudio.h"

// Registers plugin
REGISTER_PLUGIN

NRTAudioConfig::NRTAudioConfig()
{
	onoff = 0;
}

NRTAudioSwitch::NRTAudioSwitch(NRTAudio *plugin, 
	int x,
	int y)
 : BC_Radial(x, y, plugin->config.onoff, _("Clear"))
{
	this->plugin = plugin;
}

int NRTAudioSwitch::handle_event()
{
	plugin->config.onoff = plugin->config.onoff ? 0 : 1;
	update(plugin->config.onoff);
	plugin->send_configure_change();
	return 1;
}

/*
 * The gui of the plugin
 * Using the constuctor macro is mandatory
 * The constructor macro must be near the end of constructor after
 * creating the elements of gui.
 */
NRTAudioWindow::NRTAudioWindow(NRTAudio *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y,
	200,
	50)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Status:")));
	add_subwindow(toggle = new NRTAudioSwitch(plugin,
		x + 60,
		y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void NRTAudioWindow::update()
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
NRTAudio::NRTAudio(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

NRTAudio::~NRTAudio()
{
	PLUGIN_DESTRUCTOR_MACRO
}

/*
 * Creates most of the api methods of the plugin
 */
PLUGIN_CLASS_METHODS

void NRTAudio::load_defaults()
{
// Loads defaults from the named rc-file
	defaults = load_defaults_file("nullrtaudio.rc");

	config.onoff = defaults->get("ONOFF", config.onoff);
}

void NRTAudio::save_defaults()
{
	defaults->update("ONOFF", config.onoff);
	defaults->save();
}

/*
 * Saves the values of the parameters to the project
 */
void NRTAudio::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("NRTAUDIO");
	output.tag.set_property("ONOFF", config.onoff);
	output.append_tag();
	output.tag.set_title("/NRTAUDIO");
	output.append_tag();
	keyframe->set_data(output.string);
}

void NRTAudio::read_data(KeyFrame *keyframe)
{
	FileXML input;

	if(!keyframe)
		return;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("NRTAUDIO"))
		{
			config.onoff = input.tag.get_property("ONOFF", config.onoff);
		}
	}
}

/*
 * Loads parameters from keyframe
 */
int NRTAudio::load_configuration()
{
	int prev_val = config.onoff;

	read_data(get_prev_keyframe(source_pts));
	return need_reconfigure || !(prev_val == config.onoff);
}

/*
 * Process frame
 * multichannel plugin gets here arrays of pointers
 * prototype: void process_tmpframes(AFrame **frame)
 *    - number of channels is total_in_buffers
 */
AFrame *NRTAudio::process_tmpframe(AFrame *frame)
{
	if(load_configuration())
		update_gui();

	if(config.onoff)
		memset(frame->buffer, 0, frame->get_length() * sizeof(double));
	return frame;
}

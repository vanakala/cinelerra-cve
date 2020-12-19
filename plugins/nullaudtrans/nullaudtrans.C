// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2012 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "aframe.h"
#include "bchash.h"
#include "bctitle.h"
#include "filexml.h"
#include "picon_png.h"
#include "nullaudtrans.h"

#include <string.h>

// Registers plugin
REGISTER_PLUGIN

NASwitch::NASwitch(NATransition *plugin, 
	NATransitionWindow *window,
	int x,
	int y)
 : BC_Radial(x, y, plugin->onoff, _("Clear"))
{
	this->plugin = plugin;
	this->window = window;
}

int NASwitch::handle_event()
{
	plugin->onoff = plugin->onoff ? 0 : 1;
	update(plugin->onoff);
	plugin->send_configure_change();
	return 0;
}

/*
 * The gui of the plugin
 * Using the constuctor macro is mandatory
 * The constructor macro must be near the end of constructor after
 * creating the elements of gui.
 */
NATransitionWindow::NATransitionWindow(NATransition *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y,
	200,
	50)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Status:")));
	add_subwindow(toggle = new NASwitch(plugin,
		this,
		x + 60,
		y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

/*
 * All methods of the plugin thread class
 */
PLUGIN_THREAD_METHODS

/*
 * The processor of the plugin
 * Using the constructor and the destructor macro is mandatory
 */
NATransition::NATransition(PluginServer *server)
 : PluginAClient(server)
{
	onoff = 1;
	PLUGIN_CONSTRUCTOR_MACRO
}

NATransition::~NATransition()
{
	PLUGIN_DESTRUCTOR_MACRO
}

/*
 * Creates most of the api methods of the plugin
 */
PLUGIN_CLASS_METHODS

void NATransition::load_defaults()
{
// Loads defaults from the named rc-file
	defaults = load_defaults_file("nullaudiotransition.rc");

	onoff = defaults->get("ONOFF", onoff);
}

void NATransition::save_defaults()
{
	defaults->update("ONOFF", onoff);
	defaults->save();
}

/*
 * Saves the values of the parameters to the project
 */
void NATransition::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("NATRANSITION");
	output.tag.set_property("ONOFF", onoff);
	output.append_tag();
	output.tag.set_title("/NATRANSITION");
	output.append_tag();
	keyframe->set_data(output.string);
}

/*
 * Reads values from the keyframe of the project
 */
void NATransition::read_data(KeyFrame *keyframe)
{
	FileXML input;

	if(!keyframe)
		return;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("NATRANSITION"))
		{
			onoff = input.tag.get_property("ONOFF", onoff);
		}
	}
}

/*
 * Loads parameters from keyframe
 */
int NATransition::load_configuration()
{
	read_data(get_prev_keyframe(source_pts));
	return 0;
}

/*
 * Here incoming and outgoing frames must me mixed into
 * the outgoing frame
 */
void NATransition::process_realtime(AFrame *incoming, AFrame *outgoing)
{
	load_configuration();

	if(onoff)
		memset(outgoing->buffer, 0, outgoing->get_length() * sizeof(double));
}

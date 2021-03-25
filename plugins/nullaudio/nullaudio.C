// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2012 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "bchash.h"
#include "bctitle.h"
#include "mainprogress.h"
#include "picon_png.h"
#include "aframe.h"
#include "nullaudio.h"

#include <stdint.h>
#include <string.h>

// Registers plugin
REGISTER_PLUGIN

NullAudioSwitch::NullAudioSwitch(NullAudio *plugin, 
	int x,
	int y)
 : BC_Radial(x, y, plugin->onoff,
	_("Clear"))
{
	this->plugin = plugin;
}

int NullAudioSwitch::handle_event()
{
	plugin->onoff = plugin->onoff ? 0 : 1;
	update(plugin->onoff);
	return 0;
}

/*
 * The gui of the plugin
 * Using the constuctor macro is mandatory
 * The constructor macro must be near the end of constructor after
 * creating the elements of gui.
 */
NullAudioWindow::NullAudioWindow(NullAudio *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	200,
	100)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Status:")));
	add_subwindow(new NullAudioSwitch(plugin,
		x + 60,
		y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

/*
 * The processor of the plugin
 * Using the constructor and the destructor macro is mandatory
 */
NullAudio::NullAudio(PluginServer *server)
 : PluginAClient(server)
{
	onoff = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

NullAudio::~NullAudio()
{
	PLUGIN_DESTRUCTOR_MACRO
}

/*
 * Creates most of the api methods of the plugin
 */
PLUGIN_CLASS_METHODS

void NullAudio::load_defaults()
{
// Loads defaults from the named rc-file
	defaults = load_defaults_file("nullaudio.rc");

	onoff = defaults->get("ONOFF", onoff);
}

void NullAudio::save_defaults()
{
	defaults->update("ONOFF", onoff);
	defaults->save();
}

/*
 * Pull frame and apply modifications
 * Multichannel plugin has the same interface -
 *  it receives multiple frames
 *    - number of frames is total_in_buffers
 */
int NullAudio::process_loop(AFrame **frames)
{
	AFrame *frame = *frames;

	if(onoff)
		memset(frame->buffer, 0, frame->get_length() * sizeof(double));
	return 0; // return value is not used
}

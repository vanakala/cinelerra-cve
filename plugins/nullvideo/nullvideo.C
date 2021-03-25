// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2012 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "bchash.h"
#include "bctitle.h"
#include "mainprogress.h"
#include "picon_png.h"
#include "vframe.h"
#include "nullvideo.h"

#include <stdint.h>
#include <string.h>

// Registers plugin
REGISTER_PLUGIN


NullVideoSwitch::NullVideoSwitch(NullVideo *plugin, 
	int x,
	int y)
 : BC_Radial(x,
	y,
	plugin->onoff,
	_("Clear"))
{
	this->plugin = plugin;
}

int NullVideoSwitch::handle_event()
{
	plugin->onoff = plugin->onoff ? 0 : 1;
	update(plugin->onoff);
	return 1;
}

/*
 * The gui of the plugin
 * Using the constuctor macro is mandatory
 * The constructor macro must be near the end of constructor after
 * creating the elements of gui.
 */
NullVideoWindow::NullVideoWindow(NullVideo *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	200,
	100)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Status:")));
	add_subwindow(new NullVideoSwitch(plugin,
		x + 60,
		y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

/*
 * The processor of the plugin
 * Using the constructor and the destructor macro is mandatory
 */
NullVideo::NullVideo(PluginServer *server)
 : PluginVClient(server)
{
	onoff = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

NullVideo::~NullVideo()
{
	PLUGIN_DESTRUCTOR_MACRO
}

/*
 * Creates most of the api methods of the plugin
 */
PLUGIN_CLASS_METHODS

void NullVideo::load_defaults()
{
// Loads defaults from the named rc-file
	defaults = load_defaults_file("nullvideo.rc");

	onoff = defaults->get("ONOFF", onoff);
}

void NullVideo::save_defaults()
{
	defaults->update("ONOFF", onoff);
	defaults->save();
}

/*
 * Pull frame and apply modifications
 * multichannel plugin gets here arrays of pointers
 *    - number of channels is total_in_buffers
 */
int NullVideo::process_loop(VFrame **frames)
{
	VFrame *frame = *frames;

	if(onoff)
		frame->clear_frame();

	return 0; // Not used
}

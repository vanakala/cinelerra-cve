// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "bctoggle.h"
#include "mwindow.h"
#include "plugin.h"
#include "plugintoggles.h"
#include "theme.h"
#include "vframe.h"


PluginOn::PluginOn(int x, int y, Plugin *plugin)
 : BC_Toggle(x, y, theme_global->get_image_set("plugin_on"), plugin->on)
{
	this->plugin = plugin;
	in_use = 1;
}

int PluginOn::calculate_w()
{
	return theme_global->get_image_set("plugin_on")[0]->get_w();
}

void PluginOn::update(int x, int y, Plugin *plugin)
{
	BC_Toggle::set_value(plugin->on, 0);
	reposition_window(x, y);
	this->plugin = plugin;
	in_use = 1;
}

int PluginOn::handle_event()
{
	plugin->on = get_value();
	mwindow_global->sync_parameters();
	return 1;
}


PluginShow::PluginShow(int x, int y, Plugin *plugin)
 : BC_Toggle(x, y, theme_global->get_image_set("plugin_show"), plugin->show)
{
	this->plugin = plugin;
	in_use = 1;
}

int PluginShow::calculate_w()
{
	return theme_global->get_image_set("plugin_show")[0]->get_w();
}

void PluginShow::update(int x, int y, Plugin *plugin)
{
	BC_Toggle::set_value(plugin->show, 0);
	reposition_window(x, y);
	this->plugin = plugin;
	in_use = 1;
}

int PluginShow::handle_event()
{
	mwindow_global->show_plugin(plugin);
	BC_Toggle::set_value(plugin->show, 0);
	return 1;
}

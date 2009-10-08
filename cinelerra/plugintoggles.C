
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

#include "mwindow.h"
#include "plugin.h"
#include "plugintoggles.h"
#include "theme.h"



PluginOn::PluginOn(MWindow *mwindow, int x, int y, Plugin *plugin)
 : BC_Toggle(x, 
 		y, 
		mwindow->theme->get_image_set("plugin_on"),
		plugin->on)
{
	this->mwindow = mwindow;
	this->plugin = plugin;
	in_use = 1;
}

int PluginOn::calculate_w(MWindow *mwindow)
{
	return mwindow->theme->get_image_set("plugin_on")[0]->get_w();
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
	unlock_window();
	mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_EDL);
	lock_window("PluginOn::handle_event");
	return 1;
}




PluginShow::PluginShow(MWindow *mwindow, int x, int y, Plugin *plugin)
 : BC_Toggle(x, 
 		y, 
		mwindow->theme->get_image_set("plugin_show"),
		plugin->show)
{
	this->mwindow = mwindow;
	this->plugin = plugin;
	in_use = 1;
}

int PluginShow::calculate_w(MWindow *mwindow)
{
	return mwindow->theme->get_image_set("plugin_show")[0]->get_w();
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
	unlock_window();
	if(get_value()) 
		mwindow->show_plugin(plugin);
	else
		mwindow->hide_plugin(plugin, 1);
	lock_window("PluginShow::handle_event");
	return 1;
}






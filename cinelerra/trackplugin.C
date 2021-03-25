// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "bcpixmap.h"
#include "bcsubwindow.h"
#include "bcresources.h"
#include "mwindow.h"
#include "plugin.h"
#include "trackplugin.h"
#include "theme.h"
#include "vframe.h"

TrackPlugin::TrackPlugin(int x, int y, int w, int h, Plugin *plugin)
 : BC_SubWindow(x, y, w, h)
{
	this->plugin = plugin;
	plugin_on = 0;
	plugin_show = 0;
	background = 0;
	drawn_x = drawn_y = drawn_w = drawn_h = -1;
}

void TrackPlugin::show()
{
	redraw(get_x(), get_y(), get_w(), get_h());
}

TrackPlugin::~TrackPlugin()
{
	if(!get_deleting())
	{
		delete plugin_on;
		delete plugin_show;
	}
	if(plugin)
		plugin->trackplugin = 0;
}

void TrackPlugin::redraw(int x, int y, int w, int h)
{
	char string[BCTEXTLEN];
	int text_left = 5;

	if(drawn_w != w || drawn_h != h)
	{
		reposition_window(x, y, w, h);
		draw_3segmenth(0, 0, w, theme_global->get_image("plugin_bg_data"), 0);
		plugin->calculate_title(string);
		set_color(get_resources()->default_text_color);
		set_font(MEDIUMFONT_3D);
		draw_text(text_left, get_text_ascent(MEDIUMFONT_3D) + 2,
			string, strlen(string), 0);

		int toggle_x = w - PluginOn::calculate_w() - 10;

		if(toggle_x < 0)
		{
			delete plugin_on;
			plugin_on = 0;
		}
		else
		{
			if(plugin_on)
				plugin_on->update(toggle_x);
			else
				add_subwindow(plugin_on = new PluginOn(toggle_x, plugin));
		}

		if(plugin->plugin_type == PLUGIN_STANDALONE)
		{
			toggle_x -= PluginShow::calculate_w() + 10;
			if(toggle_x < 0)
			{
				delete plugin_show;
				plugin_show = 0;
			}
			else
			{
				if(plugin_show)
					plugin_show->update(toggle_x);
				else
					add_subwindow(plugin_show = new PluginShow(toggle_x, plugin));
			}
		}
		drawn_w = w;
		drawn_h = h;
		drawn_x = x;
		drawn_y = y;
		flash();
	}
	if(drawn_x != x || drawn_y != y)
	{
		reposition_window(x, y, w, h);
		drawn_x = x;
		drawn_y = y;
	}
}

void TrackPlugin::update(int x, int y, int w, int h)
{
	redraw(x, y, w, h);
}

void TrackPlugin::update_toggles()
{
	int x = get_x();

	if(plugin_on)
		plugin_on->update(x);
	if(plugin_show)
		plugin_show->update(x);
}

PluginOn::PluginOn(int x, Plugin *plugin)
 : BC_Toggle(x, 0, theme_global->get_image_set("plugin_on"), plugin->on)
{
	this->plugin = plugin;
}

int PluginOn::calculate_w()
{
	return theme_global->get_image_set("plugin_on")[0]->get_w();
}

void PluginOn::update(int x)
{
	set_value(plugin->on, 0);
	reposition_window(x, 0);
}

int PluginOn::handle_event()
{
	plugin->on = get_value();
	mwindow_global->sync_parameters();
	return 1;
}


PluginShow::PluginShow(int x, Plugin *plugin)
 : BC_Toggle(x, 0, theme_global->get_image_set("plugin_show"), plugin->show)
{
	this->plugin = plugin;
}

int PluginShow::calculate_w()
{
	return theme_global->get_image_set("plugin_show")[0]->get_w();
}

void PluginShow::update(int x)
{
	set_value(plugin->show, 0);
	reposition_window(x, 0);
}

int PluginShow::handle_event()
{
	mwindow_global->show_plugin(plugin);
	set_value(plugin->show, 0);
	return 1;
}

// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "bcpixmap.h"
#include "bcsubwindow.h"
#include "bcresources.h"
#include "cinelerra.h"
#include "cursors.h"
#include "cwindow.h"
#include "edl.h"
#include "edlsession.h"
#include "keyframe.h"
#include "keyframepopup.h"
#include "keyframes.h"
#include "localsession.h"
#include "mainsession.h"
#include "mwindow.h"
#include "plugin.h"
#include "pluginpopup.h"
#include "trackplugin.h"
#include "trackcanvas.h"
#include "theme.h"
#include "vframe.h"

TrackPlugin::TrackPlugin(int x, int y, int w, int h,
	Plugin *plugin, TrackCanvas *canvas)
 : BC_SubWindow(x, y, w, h)
{
	this->plugin = plugin;
	this->canvas = canvas;
	plugin_on = 0;
	plugin_show = 0;
	keyframe_pixmap = 0;
	num_keyframes = 0;
	keyframe_width = 0;
	drawn_x = drawn_y = drawn_w = drawn_h = -1;
}

void TrackPlugin::show()
{
	set_cursor(canvas->default_cursor());
	redraw(get_x(), get_y(), get_w(), get_h());
}

TrackPlugin::~TrackPlugin()
{
	if(!get_deleting())
	{
		delete plugin_on;
		delete plugin_show;
	}
	delete keyframe_pixmap;
	if(plugin)
		plugin->trackplugin = 0;
}

void TrackPlugin::redraw(int x, int y, int w, int h)
{
	char string[BCTEXTLEN];
	int text_left = 5;
	int redraw = 0;
	int kcount = 0;
	int kx;

	if(!edlsession->keyframes_visible && num_keyframes)
	{
		redraw++;
		num_keyframes = 0;
		for(KeyFrame *keyframe = (KeyFrame*)plugin->keyframes->first;
				keyframe; keyframe = (KeyFrame*)keyframe->next)
			keyframe->drawing(-1);
	}
	else
	{
		if(plugin->keyframes->first &&
			plugin->keyframes->first != plugin->keyframes->last)
		{
			for(KeyFrame *keyframe = (KeyFrame*)plugin->keyframes->first;
				keyframe; keyframe = (KeyFrame*)keyframe->next)
			{
				kx = (keyframe->pos_time - master_edl->local_session->view_start_pts) /
					master_edl->local_session->zoom_time - x;
				if(!keyframe->has_drawn(kx))
				{
					redraw++;
					break;
				}
				kcount++;
			}
		}
	}

	if(drawn_w != w || drawn_h != h || redraw)
	{
		reposition_window(x, y, w, h);
		draw_3segmenth(0, 0, w, theme_global->get_image("plugin_bg_data"), 0);
		plugin->calculate_title(string);
		set_color(get_resources()->default_text_color);
		set_font(MEDIUMFONT_3D);
		draw_text(text_left, get_text_ascent(MEDIUMFONT_3D) + 2,
			string, strlen(string), 0);
		redraw = 1;
	}

	if(edlsession->keyframes_visible && (kcount != num_keyframes || redraw))
	{
		if(!keyframe_pixmap)
		{
			keyframe_pixmap = new BC_Pixmap(this,
				theme_global->keyframe_data, PIXMAP_ALPHA);
			keyframe_width = keyframe_pixmap->get_w();
		}
		int ky = (h - keyframe_pixmap->get_h()) / 2;

		num_keyframes = 0;

		for(KeyFrame *keyframe = (KeyFrame*)plugin->keyframes->first;
			keyframe; keyframe = (KeyFrame*)keyframe->next)
		{
			kx = (keyframe->pos_time - master_edl->local_session->view_start_pts) /
				master_edl->local_session->zoom_time - x;
			if(redraw || !keyframe->has_drawn(kx))
			{
				draw_pixmap(keyframe_pixmap, kx, ky);
				num_keyframes++;
			}
		}
	}

	if(drawn_w != w || drawn_h != h || redraw)
	{
		int toggle_x = w - PluginOn::calculate_w() - 10;

		if(toggle_x < 0)
		{
			delete plugin_on;
			plugin_on = 0;
		}
		else
		{
			if(plugin_on)
				plugin_on->reposition_window(toggle_x, 0);
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
					plugin_show->reposition_window(toggle_x, 0);
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
	if(plugin_on)
		plugin_on->update();
	if(plugin_show)
		plugin_show->update();
}

int TrackPlugin::cursor_motion_event()
{
	if(is_event_win() && mainsession->current_operation == NO_OPERATION)
	{
		int new_cursor = canvas->default_cursor();
		int cursor_x = get_cursor_x();

		if(cursor_x < HANDLE_W)
			new_cursor = LEFT_CURSOR;
		else if(cursor_x >= drawn_w - HANDLE_W)
			new_cursor = RIGHT_CURSOR;
		else if(num_keyframes)
		{
			for(KeyFrame *keyframe = (KeyFrame*)plugin->keyframes->first;
				keyframe; keyframe = (KeyFrame*)keyframe->next)
			{
				int kx = keyframe->get_pos_x();
				if(cursor_x >= kx && cursor_x <= kx + keyframe_width)
				{
					new_cursor = ARROW_CURSOR;
					break;
				}
			}
		}

		if(new_cursor != get_cursor())
			set_cursor(new_cursor);
		return 1;
	}
	return 0;
}

int TrackPlugin::button_press_event()
{
	int cursor_x, cursor_y;

	cursor_x = get_cursor_x();
	cursor_y = get_cursor_y();

	if(is_event_win())
	{
		int new_cursor = canvas->default_cursor();
		int cursor_x = get_cursor_x();

		if(cursor_x < HANDLE_W)
			new_cursor = LEFT_CURSOR;
		else if(cursor_x >= drawn_w - HANDLE_W)
			new_cursor = RIGHT_CURSOR;
		else
		{
			if(get_buttonpress() == 3)
			{
				if(num_keyframes)
				{
					for(KeyFrame *keyframe = (KeyFrame*)plugin->keyframes->first;
						keyframe; keyframe = (KeyFrame*)keyframe->next)
					{
						int kx = keyframe->get_pos_x();

						if(cursor_x >= kx && cursor_x <= kx + keyframe_width)
						{
							canvas->keyframe_menu->update(plugin, keyframe);
							canvas->keyframe_menu->activate_menu();
							return 1;
						}
					}
				}
				canvas->plugin_menu->update(plugin);
				canvas->plugin_menu->activate_menu();
				return 1;
			}
			else if(get_double_click())
			{
				master_edl->local_session->set_selectionstart(plugin->get_pts());
				master_edl->local_session->set_selectionend(plugin->end_pts());
				mwindow_global->cwindow->update(WUPD_POSITION | WUPD_TIMEBAR);
				mwindow_global->update_plugin_guis();
				mwindow_global->update_gui(WUPD_PATCHBAY |
					WUPD_CURSOR | WUPD_ZOOMBAR | WUPD_TOGLIGHTS);
				return 1;
			}
			return 0;
		}

		if(new_cursor != get_cursor())
			set_cursor(new_cursor);
		return 1;
	}
	return 0;
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

void PluginOn::update()
{
	set_value(plugin->on, 1);
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

void PluginShow::update()
{
	set_value(plugin->show, 1);
}

int PluginShow::handle_event()
{
	mwindow_global->show_plugin(plugin);
	set_value(plugin->show, 0);
	return 1;
}

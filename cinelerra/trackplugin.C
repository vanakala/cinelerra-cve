// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcdragwindow.h"
#include "bcsignals.h"
#include "bcpixmap.h"
#include "bcsubwindow.h"
#include "bcresources.h"
#include "cinelerra.h"
#include "clip.h"
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
#include "mwindowgui.h"
#include "plugin.h"
#include "pluginpopup.h"
#include "pluginserver.h"
#include "track.h"
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
	num_keyframes = 0;
	keyframe_width = canvas->keyframe_pixmap->get_w();
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
	if(plugin)
		plugin->trackplugin = 0;
}

void TrackPlugin::redraw(int x, int y, int w, int h)
{
	char string[BCTEXTLEN];
	int text_left = 5;
	int redraw = 0;
	int kcount = 0;
	int total_kf;
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
		total_kf = plugin->keyframes->total();

		if(total_kf > 1)
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
	if(edlsession->keyframes_visible && total_kf > 1 &&
		(kcount != num_keyframes || redraw))
	{
		int ky = (h - canvas->keyframe_pixmap->get_h()) / 2;
		num_keyframes = 0;

		for(KeyFrame *keyframe = (KeyFrame*)plugin->keyframes->first;
			keyframe; keyframe = (KeyFrame*)keyframe->next)
		{
			kx = (keyframe->pos_time - master_edl->local_session->view_start_pts) /
				master_edl->local_session->zoom_time - x;
			if(redraw || !keyframe->has_drawn(kx))
			{
				draw_pixmap(canvas->keyframe_pixmap, kx, ky);
				keyframe->drawing(kx);
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
	int buttonpress;

	if(is_event_win() && (buttonpress = get_buttonpress()) < 4)
	{
		int new_cursor = canvas->default_cursor();
		int cursor_x = get_cursor_x();

		if(cursor_x < HANDLE_W)
			new_cursor = LEFT_CURSOR;
		else if(cursor_x >= drawn_w - HANDLE_W)
			new_cursor = RIGHT_CURSOR;
		else
		{
			if(buttonpress == 3)
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
			else if(get_double_click() && buttonpress == 1)
			{
				master_edl->local_session->set_selection(plugin->get_pts(),
					plugin->end_pts());
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

int TrackPlugin::drag_start_event()
{
	if(is_event_win() && plugin->track->record)
	{
		if(edlsession->editing_mode == EDITING_ARROW)
		{
			int cx, cy;
			PluginServer *server;
			VFrame *frame;

			switch(plugin->track->data_type)
			{
			case TRACK_AUDIO:
				mainsession->current_operation = DRAG_AEFFECT_COPY;
				break;
			case TRACK_VIDEO:
				mainsession->current_operation = DRAG_VEFFECT_COPY;
				break;
			}

			mainsession->drag_plugin = plugin;
// Create picon
			switch(plugin->plugin_type)
			{
			case PLUGIN_STANDALONE:
				if(server = plugin->plugin_server)
				{
					frame = server->picon;
					break;

				}
				// Fall through
			case PLUGIN_SHAREDPLUGIN:
			case PLUGIN_SHAREDMODULE:
				frame = theme_global->get_image("clip_icon");
				break;
			}
			BC_Resources::get_abs_cursor(&cx, &cy);
			canvas->drag_popup = new BC_DragWindow(canvas->gui,
				frame,
				cx - frame->get_w() / 2,
				cy - frame->get_h() / 2);
			return 1;
		}
		else if(edlsession->editing_mode == EDITING_IBEAM)
		{
			int cursor_x = get_cursor_x();

			if(cursor_x < HANDLE_W)
			{
				canvas->start_pluginhandle_drag(plugin->get_pts(),
					DRAG_PLUGINHANDLE1, HANDLE_LEFT, plugin);
				return 1;
			}
			if(cursor_x >= drawn_w - HANDLE_W)
			{
				canvas->start_pluginhandle_drag(plugin->end_pts(),
					DRAG_PLUGINHANDLE1, HANDLE_RIGHT, plugin);
				return 1;
			}
			if(num_keyframes)
			{
				for(KeyFrame *keyframe = (KeyFrame*)plugin->keyframes->first;
					keyframe; keyframe = (KeyFrame*)keyframe->next)
				{
					if(keyframe == plugin->keyframes->first)
						continue;

					int kx = keyframe->get_pos_x();

					if(cursor_x >= kx && cursor_x <= kx + keyframe_width)
					{
						mainsession->current_operation = DRAG_PLUGINKEY;
						mainsession->drag_auto = keyframe;
						mainsession->drag_start_postime = keyframe->pos_time;
						mainsession->drag_plugin = plugin;
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

void TrackPlugin::drag_motion_event()
{
	if(mainsession->current_operation == DRAG_PLUGINKEY)
	{
		KeyFrame *keyframe = (KeyFrame*)mainsession->drag_auto;
		int cx = get_cursor_x();
		ptstime minpos, maxpos, new_pos;

		plugin->keyframes->drag_limits(keyframe, &minpos, &maxpos);
		new_pos = (cx - get_drag_x()) *
			master_edl->local_session->zoom_time;
		new_pos = master_edl->align_to_frame(mainsession->drag_start_postime + new_pos);
		CLAMP(new_pos , minpos, maxpos);
		if(!EQUIV(new_pos, keyframe->pos_time))
		{
			keyframe->pos_time = new_pos;
			keyframe->drawing(-1);
			redraw(drawn_x, drawn_y, drawn_w, drawn_h);
		}
	}
	else
		canvas->drag_motion();
}

void TrackPlugin::drag_stop_event()
{
	canvas->drag_stop();
	delete canvas->drag_popup;
	canvas->drag_popup = 0;
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

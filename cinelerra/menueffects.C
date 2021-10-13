
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

#include "asset.h"
#include "bctitle.h"
#include "bclistboxitem.h"
#include "cinelerra.h"
#include "clip.h"
#include "datatype.h"
#include "bchash.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "mainerror.h"
#include "file.h"
#include "filexml.h"
#include "keyframe.h"
#include "keys.h"
#include "labels.h"
#include "language.h"
#include "loadmode.h"
#include "localsession.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "menueffects.h"
#include "paramlist.h"
#include "plugin.h"
#include "pluginclient.h"
#include "plugindb.h"
#include "pluginserver.h"
#include "preferences.h"
#include "render.h"
#include "renderprofiles.h"
#include "track.h"
#include "tracks.h"
#include "theme.h"


MenuEffects::MenuEffects()
 : BC_MenuItem(_("Render effect..."))
{
}

int MenuEffects::handle_event()
{
	thread->set_title(0);
	thread->start();
	return 1;
}


MenuEffectThread::MenuEffectThread()
{
	title[0] = 0;
}

void MenuEffectThread::set_title(const char *title)
{
	if(title && title[0])
		strcpy(this->title, title);
	else
		this->title[0] = 0;
}

void MenuEffectThread::get_derived_attributes(Asset *asset)
{
	Paramlist *params;
	FileXML file;
	const char *pfname;
	char path[BCTEXTLEN];
	BC_Hash *defaults = mwindow_global->defaults;

	edlsession->configuration_path(RENDERCONFIG_DIR, path);
	RenderProfile::chk_profile_dir(path);
	asset->set_renderprofile(path, profile_name);
	RenderProfile::chk_profile_dir(asset->renderprofile_path);

	asset->load_defaults(defaults, def_prefix,
		ASSET_FORMAT | ASSET_COMPRESSION | ASSET_PATH | ASSET_BITS);
	load_mode = defaults->get("RENDER_EFFECT_LOADMODE", LOADMODE_PASTE);
	strategy = defaults->get("RENDER_EFFECT_STRATEGY", 0);

	delete asset->render_parameters;
	asset->render_parameters = 0;

	if(!file.read_from_file(asset->profile_config_path("ProfilData.xml", path), 1) && !file.read_tag())
	{
		params = new Paramlist("");
		params->load_list(&file);
		asset->load_defaults(params,
			ASSET_FORMAT | ASSET_COMPRESSION | ASSET_PATH | ASSET_BITS);
		asset->render_parameters = params;
		load_mode = params->get("loadmode", load_mode);
		strategy = params->get("strategy", strategy);
	}
	// Fix asset for media type only
	if(effect_type & SUPPORTS_AUDIO)
	{
		if(!(File::supports(asset->format) & SUPPORTS_AUDIO))
			asset->format = FILE_WAV;
		asset->remove_stream_type(STRDSC_VIDEO);
		asset->create_render_stream(STRDSC_AUDIO);
	}
	else if(effect_type & SUPPORTS_VIDEO)
	{
		if(!(File::supports(asset->format) & SUPPORTS_VIDEO))
			asset->format = FILE_MOV;
		asset->remove_stream_type(STRDSC_AUDIO);
		asset->create_render_stream(STRDSC_VIDEO);
	}
}

void MenuEffectThread::save_derived_attributes(Asset *asset)
{
	Paramlist params("ProfilData");
	Param *pp;
	FileXML file;
	char path[BCTEXTLEN];
	BC_Hash *defaults = mwindow_global->defaults;

	asset->save_defaults(&params, ASSET_FORMAT | ASSET_COMPRESSION | ASSET_PATH | ASSET_BITS);
	params.append_param("loadmode", load_mode);
	params.append_param("strategy", strategy);
	if(asset->render_parameters)
		params.remove_equiv(asset->render_parameters);
	else
		asset->render_parameters = new Paramlist("ProfilData");

	if(params.total() > 0)
	{
		for(pp = params.first; pp; pp = pp->next)
			asset->render_parameters->set(pp);
		asset->render_parameters->save_list(&file);
		file.write_to_file(asset->profile_config_path("ProfilData.xml", path));
	}
	// Remove old version keys
	defaults->delete_keys_prefix(def_prefix);
	defaults->delete_key("RENDER_EFFECT_LOADMODE");
	defaults->delete_key("RENDER_EFFECT_STRATEGY");
}

// for recent effect menu items and running new effects
// prompts for an effect if title is blank
void MenuEffectThread::run()
{
// get stuff from main window
	ArrayList<BC_ListBoxItem*> plugin_list;
	ArrayList<PluginServer*> local_plugindb;
	int result = 0;
// Default configuration
	Asset *default_asset = new Asset;

// check for recordable tracks
	if(!get_recordable_tracks(default_asset))
	{
		errorbox(_("No recordable tracks specified."));
		delete default_asset;
		return;
	}

// check for plugins
	if(!plugindb.count())
	{
		errorbox(_("No plugins available."));
		delete default_asset;
		return;
	}

// get default attributes for output file
	get_derived_attributes(default_asset);

// get plugin information
	int need_plugin = !title[0];

// generate a list of plugins for the window
	if(need_plugin)
	{
		plugindb.fill_plugindb(default_asset->stream_count(STRDSC_AUDIO),
			default_asset->stream_count(STRDSC_VIDEO),
			-1,
			-1,
			0,
			0,
			local_plugindb);

		for(int i = 0; i < local_plugindb.total; i++)
		{
			plugin_list.append(new BC_ListBoxItem(_(local_plugindb.values[i]->title)));
		}
	}

// find out which effect to run and get output file
	int plugin_number;
	int cx, cy;

	mwindow_global->get_abs_cursor_pos(&cx, &cy);
	MenuEffectWindow window(this,
		need_plugin ? &plugin_list : 0,
		cx, cy,
		default_asset);
	result = window.run_window();
	plugin_number = window.result;

// save defaults
	save_derived_attributes(default_asset);

// get plugin server to use and delete the plugin list
	PluginServer *plugin_server = 0;

	if(need_plugin)
	{
		plugin_list.remove_all_objects();

		if(plugin_number > -1)
			plugin_server = local_plugindb.values[plugin_number];
	}
	else
		plugin_server = plugindb.get_pluginserver(title, track_type, 0);

// Update the  most recently used effects and copy the plugin server.
	if(plugin_server)
		fix_menu(plugin_server->title);

	if(!result && !strlen(default_asset->path))
	{
		result = 1;        // no output path given
		errorbox(_("No output file specified."));
	}

	if(!result && !plugin_server)
	{
		result = 1;        // no output path given
		errorbox(_("No effect selected."));
	}

// Configuration for realtime plugins.
	KeyFrame plugin_data;

// get selection to render
// Range
	ptstime total_start, total_end;
	int range_type;

	total_start = master_edl->local_session->get_selectionstart();

	if(PTSEQU(master_edl->local_session->get_selectionend(), total_start))
	{
		total_end = master_edl->total_length();
		total_start = 0;
		range_type = RANGE_PROJECT;
	}
	else
	{
		total_end = master_edl->local_session->get_selectionend();
		range_type = RANGE_SELECTION;
	}

	if(!result &&  (total_end - total_start) <= EPSILON)
	{
		result = 1;
		errorbox(_("No selected range to process."));
	}

// ========================= get keyframe from user
	if(!result)
	{
		PluginClient *client = plugin_server->open_plugin(0, 0);
// ========================= realtime plugin 
// no get_parameters
		if(plugin_server->realtime)
		{
			int cx, cy;
			mwindow_global->get_abs_cursor_pos(&cx, &cy);
// Open a prompt GUI
			MenuEffectPrompt prompt(cx, cy);

			client->set_prompt(&prompt);
// Must set parameters since there is no plugin object to draw from.
			client->plugin_get_parameters(1);
			client->plugin_show_gui();

// wait for user input
			result = prompt.run_window();

// Close plugin.
			client->hide_gui();
			client->save_data(&plugin_data);
			client->set_prompt(0);
			realtime = 1;
		}
		else
// ============================non realtime plugin 
		{
			client->update_display_title();
			result = client->plugin_get_parameters(
				get_recordable_tracks(default_asset));

			realtime = 0;
		}
		plugin_server->close_plugin(client);

		if(plugin_server->audio)
			default_asset->create_render_stream(STRDSC_AUDIO);
		if(plugin_server->video)
			default_asset->create_render_stream(STRDSC_VIDEO);
	}

	if(!result)
	{
		ptstime min_track_length = master_edl->total_length();

		render_edl = new EDL(0);
		render_edl->update_assets(master_edl);
		render_edl->labels->copy_from(master_edl->labels);
		render_edl->local_session->copy_from(master_edl->local_session);

		if(master_edl->this_edlsession)
		{
			render_edl->this_edlsession = new EDLSession();
			render_edl->this_edlsession->copy(master_edl->this_edlsession);
		}

		for(Track *track = master_edl->first_track(); track; track = track->next)
		{
			if(track->data_type != track_type || !track->record)
				continue;

			Track *new_track = render_edl->tracks->add_track(track_type, 0, 0);

			new_track->copy_settings(track);
			new_track->edits->copy_from(track->edits);
			new_track->automation->copy_from(track->automation);

			ptstime track_dur = new_track->get_length();
			if(track_dur < min_track_length)
				min_track_length = track_dur;
		}

		Plugin *shared_master = 0;

		for(Track *track = render_edl->first_track(); track; track = track->next)
		{
			ptstime max_pts = track->get_length();

			if(max_pts > total_end)
				max_pts = total_end;

			if(max_pts < total_start)
				continue;

			if(!shared_master)
			{
				Plugin *new_plugin = track->insert_effect(plugin_server,
					0, max_pts,
					PLUGIN_STANDALONE, 0, 0);
				KeyFrame *new_keyframe = new_plugin->get_keyframe(total_start);

				new_keyframe->set_data(plugin_data.get_data());

				if(track == render_edl->first_track() &&
						plugin_server->multichannel)
				{
					new_plugin->set_end(min_track_length);
					shared_master = new_plugin;
				}
			}
			else
			if(total_start < min_track_length)
			{
				track->insert_effect(0, 0, min_track_length,
					PLUGIN_SHAREDPLUGIN, shared_master, 0);
			}
		}

		if(!render_preferences)
			render_preferences = new Preferences;
		render_preferences->copy_from(preferences_global);

		if(default_asset->single_image)
			range_type = RANGE_SINGLEFRAME;

		if(range_type == RANGE_SELECTION)
			render_edl->local_session->set_selection(total_start, total_end);

		mwindow_global->render->run_menueffects(default_asset, render_edl,
			strategy, range_type, load_mode);

		delete render_edl->this_edlsession;
		delete render_edl;
		render_edl = 0;
	}
	delete default_asset;
}


MenuEffectItem::MenuEffectItem(MenuEffects *menueffect, const char *string)
 : BC_MenuItem(string)
{
	this->menueffect = menueffect; 
}

int MenuEffectItem::handle_event()
{
	menueffect->thread->set_title(get_text());
	menueffect->thread->start();
	return 1;
}


MenuEffectWindow::MenuEffectWindow(MenuEffectThread *menueffects,
	ArrayList<BC_ListBoxItem*> *plugin_list, int absx, int absy,
	Asset *asset)
 : BC_Window(MWindow::create_title(N_("Render effect")),
		absx,
		absy - mainsession->menueffect_h / 2,
		mainsession->menueffect_w,
		mainsession->menueffect_h,
		580,
		350,
		1,
		0,
		1)
{ 
	int x, y;

	result = -1;
	this->menueffects = menueffects; 
	this->plugin_list = plugin_list; 
	this->asset = asset;
	set_icon(mwindow_global->get_window_icon());
	theme_global->get_menueffect_sizes(plugin_list ? 1 : 0);

// only add the list if needed
	if(plugin_list)
	{
		add_subwindow(list_title = new BC_Title(theme_global->menueffect_list_x,
			theme_global->menueffect_list_y,
			_("Select an effect")));
		add_subwindow(list = new MenuEffectWindowList(this, 
			theme_global->menueffect_list_x,
			theme_global->menueffect_list_y + list_title->get_h() + 5,
			theme_global->menueffect_list_w,
			theme_global->menueffect_list_h - list_title->get_h() - 5,
			plugin_list));
	}

	add_subwindow(file_title = new BC_Title(theme_global->menueffect_file_x,
		theme_global->menueffect_file_y,
		_("Select a file to render to:")));

	x = theme_global->menueffect_tools_x;
	y = theme_global->menueffect_tools_y;
	int flags = asset->stream_count(STRDSC_AUDIO) ? SUPPORTS_AUDIO : 0;
	flags |= asset->stream_count(STRDSC_VIDEO) ? SUPPORTS_VIDEO : 0;
	format_tools = new FormatTools(this, asset, x, y, flags,
		0, SUPPORTS_VIDEO,
		&menueffects->strategy);

	loadmode = new LoadMode(this, x, y, &menueffects->load_mode, 1);

	add_subwindow(new MenuEffectWindowOK(this));
	add_subwindow(new MenuEffectWindowCancel(this));
	show_window();
	flush();
}

MenuEffectWindow::~MenuEffectWindow()
{
	delete format_tools;
}

void MenuEffectWindow::resize_event(int w, int h)
{
	mainsession->menueffect_w = w;
	mainsession->menueffect_h = h;
	theme_global->get_menueffect_sizes(plugin_list ? 1 : 0);

	if(plugin_list)
	{
		list_title->reposition_window(theme_global->menueffect_list_x,
			theme_global->menueffect_list_y);
		list->reposition_window(theme_global->menueffect_list_x,
			theme_global->menueffect_list_y + list_title->get_h() + 5,
			theme_global->menueffect_list_w,
			theme_global->menueffect_list_h - list_title->get_h() - 5);
	}

	file_title->reposition_window(theme_global->menueffect_file_x,
		theme_global->menueffect_file_y);
	int x = theme_global->menueffect_tools_x;
	int y = theme_global->menueffect_tools_y;
	format_tools->reposition_window(x, y);
	loadmode->reposition_window(x, y);
}

MenuEffectWindowOK::MenuEffectWindowOK(MenuEffectWindow *window)
 : BC_OKButton(window)
{ 
	this->window = window; 
}

int MenuEffectWindowOK::handle_event() 
{ 
	if(window->plugin_list) 
		window->result = window->list->get_selection_number(0, 0); 

	window->set_done(0); 
	return 1;
}

int MenuEffectWindowOK::keypress_event() 
{ 
	if(get_keypress() == RETURN) 
	{ 
		handle_event(); 
		return 1; 
	}
	return 0;
}


MenuEffectWindowCancel::MenuEffectWindowCancel(MenuEffectWindow *window)
 : BC_CancelButton(window)
{ 
	this->window = window; 
}

int MenuEffectWindowCancel::handle_event() 
{ 
	window->set_done(1);
	return 1;
}

int MenuEffectWindowCancel::keypress_event() 
{ 
	if(get_keypress() == ESC) 
	{ 
		handle_event(); 
		return 1; 
	}
	return 0;
}


MenuEffectWindowList::MenuEffectWindowList(MenuEffectWindow *window, 
	int x, 
	int y, 
	int w, 
	int h, 
	ArrayList<BC_ListBoxItem*> *plugin_list)
 : BC_ListBox(x, 
		y,
		w, 
		h, 
		plugin_list)
{ 
	this->window = window; 
}

int MenuEffectWindowList::handle_event() 
{
	window->result = get_selection_number(0, 0);
	window->set_done(0);
	return 1;
}

#define PROMPT_TEXT _("Set up effect panel and hit \"OK\"")

MenuEffectPrompt::MenuEffectPrompt(int absx, int absy)
 : BC_Window(MWindow::create_title(N_("Effect Prompt")),
		absx - 260 / 2,
		absy - 300,
		MenuEffectPrompt::calculate_w(mwindow_global->gui),
		MenuEffectPrompt::calculate_h(mwindow_global->gui),
		MenuEffectPrompt::calculate_w(mwindow_global->gui),
		MenuEffectPrompt::calculate_h(mwindow_global->gui),
		0,
		0,
		1)
{
	int x = 10, y = 10;
	BC_Title *title;

	set_icon(mwindow_global->get_window_icon());
	add_subwindow(title = new BC_Title(x, y, PROMPT_TEXT));
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	raise_window();
	flush();
}

int MenuEffectPrompt::calculate_w(BC_WindowBase *gui)
{
	int w = BC_Title::calculate_w(gui, PROMPT_TEXT) + 10;

	w = MAX(w, BC_OKButton::calculate_w() + BC_CancelButton::calculate_w() + 30);
	return w;
}

int MenuEffectPrompt::calculate_h(BC_WindowBase *gui)
{
	int h = BC_Title::calculate_h(gui, PROMPT_TEXT);

	h += BC_OKButton::calculate_h() + 30;
	return h;
}

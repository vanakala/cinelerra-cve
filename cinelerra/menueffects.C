
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
#include "cinelerra.h"
#include "clip.h"
#include "confirmsave.h"
#include "datatype.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "mainerror.h"
#include "file.h"
#include "filexml.h"
#include "indexfile.h"
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
#include "playbackengine.h"
#include "pluginarray.h"
#include "pluginserver.h"
#include "preferences.h"
#include "render.h"
#include "renderprofiles.h"
#include "sighandler.h"
#include "theme.h"
#include "tracks.h"



MenuEffects::MenuEffects(MWindow *mwindow)
 : BC_MenuItem(_("Render effect..."))
{
	this->mwindow = mwindow;
}

int MenuEffects::handle_event()
{
	thread->set_title("");
	thread->start();
}


MenuEffectPacket::MenuEffectPacket(const char *path, ptstime start, ptstime end)
{
	this->start = start;
	this->end = end;
	strcpy(this->path, path);
}


MenuEffectThread::MenuEffectThread(MWindow *mwindow)
{
	this->mwindow = mwindow;
	title[0] = 0;
}

int MenuEffectThread::set_title(const char *title)
{
	strcpy(this->title, title);
}

void MenuEffectThread::get_derived_attributes(Asset *asset, BC_Hash *defaults)
{
	Paramlist *params;
	FileXML file;
	const char *pfname;
	char path[BCTEXTLEN];

	mwindow->edl->session->configuration_path(RENDERCONFIG_DIR, path);
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
		asset->audio_data = 1;
		asset->video_data = 0;
	}
	else if(effect_type & SUPPORTS_VIDEO)
	{
		if(!(File::supports(asset->format) & SUPPORTS_VIDEO))
			asset->format = FILE_MOV;
		asset->audio_data = 0;
		asset->video_data = 1;
	}
}

void MenuEffectThread::save_derived_attributes(Asset *asset, BC_Hash *defaults)
{
	Paramlist params("ProfilData");
	Param *pp;
	FileXML file;
	char path[BCTEXTLEN];

	asset->save_defaults(defaults, def_prefix,
		ASSET_FORMAT | ASSET_COMPRESSION | ASSET_PATH | ASSET_BITS);
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
	defaults->delete_key("RENDER_EFFECT_LOADMODE");
	defaults->delete_key("RENDER_EFFECT_STRATEGY");
}

// for recent effect menu items and running new effects
// prompts for an effect if title is blank
void MenuEffectThread::run()
{
// get stuff from main window
	ArrayList<PluginServer*> *plugindb = mwindow->plugindb;
	BC_Hash *defaults = mwindow->defaults;
	ArrayList<BC_ListBoxItem*> plugin_list;
	ArrayList<PluginServer*> local_plugindb;
	int i;
	int result = 0;
// Default configuration
	Asset *default_asset = new Asset;
// Output
	ArrayList<char*> path_list;
	path_list.set_array_delete();

// check for recordable tracks
	if(!get_recordable_tracks(default_asset))
	{
		errorbox(_("No recordable tracks specified."));
		Garbage::delete_object(default_asset);
		return;
	}

// check for plugins
	if(!plugindb->total)
	{
		errorbox(_("No plugins available."));
		Garbage::delete_object(default_asset);
		return;
	}

// get default attributes for output file
// used after completion
	get_derived_attributes(default_asset, defaults);

// get plugin information
	int need_plugin = !title[0];

// generate a list of plugins for the window
	if(need_plugin)
	{
		mwindow->create_plugindb(default_asset->audio_data, 
			default_asset->video_data, 
			-1, 
			0,
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

	MenuEffectWindow window(mwindow,
		this,
		need_plugin ? &plugin_list : 0,
		default_asset);
	result = window.run_window();
	plugin_number = window.result;

// save defaults
	save_derived_attributes(default_asset, defaults);

// get plugin server to use and delete the plugin list
	PluginServer *plugin_server = 0;
	PluginServer *plugin = 0;
	if(need_plugin)
	{
		plugin_list.remove_all_objects();
		if(plugin_number > -1)
		{
			plugin_server = local_plugindb.values[plugin_number];
			strcpy(title, plugin_server->title);
		}
	}
	else
	{
		for(int i = 0; i < plugindb->total && !plugin_server; i++)
		{
			if(!strcmp(plugindb->values[i]->title, title) &&
				((default_asset->audio_data && plugindb->values[i]->audio) ||
				(default_asset->video_data && plugindb->values[i]->video)))
			{
				plugin_server = plugindb->values[i];
				plugin_number = i;
			}
		}
	}

// Update the  most recently used effects and copy the plugin server.
	if(plugin_server)
	{
		plugin = new PluginServer(*plugin_server);
		fix_menu(title);
	}

	if(!result && !strlen(default_asset->path))
	{
		result = 1;        // no output path given
		errorbox(_("No output file specified."));
	}

	if(!result && plugin_number < 0)
	{
		result = 1;        // no output path given
		errorbox(_("No effect selected."));
	}

// Configuration for realtime plugins.
	KeyFrame plugin_data;

// get selection to render
// Range
	ptstime total_start, total_end;

	total_start = mwindow->edl->local_session->get_selectionstart();

	if(PTSEQU(mwindow->edl->local_session->get_selectionend(), total_start))
	{
		total_end = mwindow->edl->tracks->total_playable_length();
		total_start = 0;
	}
	else
		total_end = mwindow->edl->local_session->get_selectionend();

// Trick boundaries in case of a non-realtime synthesis plugin
	if(plugin && 
		!plugin->realtime && 
		PTSEQU(total_end, total_start)) total_end = total_start + one_unit();

	ptstime total_length = total_end - total_start;

	if(!result && total_length <= EPSILON)
	{
		result = 1;        // no output path given
		errorbox(_("No selected range to process."));
	}

// ========================= get keyframe from user
	if(!result)
	{
// ========================= realtime plugin 
// no get_parameters
		if(plugin->realtime)
		{
// Open a prompt GUI
			MenuEffectPrompt prompt(mwindow);
// Open the plugin GUI
			plugin->set_mwindow(mwindow);
			plugin->set_keyframe(&plugin_data);
			plugin->set_prompt(&prompt);
			plugin->open_plugin(0, mwindow->preferences, mwindow->edl, 0, -1);
// Must set parameters since there is no plugin object to draw from.
			plugin->get_parameters(total_start,
				total_end,
				1);
			plugin->show_gui();

// wait for user input
			result = prompt.run_window();

// Close plugin.
			plugin->save_data(&plugin_data);
			default_asset->sample_rate = mwindow->edl->session->sample_rate;
			default_asset->frame_rate = mwindow->edl->session->frame_rate;
			realtime = 1;
		}
		else
// ============================non realtime plugin 
		{
			plugin->set_mwindow(mwindow);
			plugin->open_plugin(0, mwindow->preferences, mwindow->edl, 0, -1);
			plugin->update_title();
			result = plugin->get_parameters(total_start,
				total_end,
				get_recordable_tracks(default_asset));
// some plugins can change the sample rate and the frame rate

			if(!result)
			{
				default_asset->sample_rate = plugin->get_samplerate();
				default_asset->frame_rate = plugin->get_framerate();
			}
			realtime = 0;
		}
		if(plugin->audio)
			default_asset->audio_data = 1;
		if(plugin->video)
			default_asset->video_data = 1;
		delete plugin;

// Should take from first recordable track
		default_asset->width = mwindow->edl->session->output_w;
		default_asset->height = mwindow->edl->session->output_h;
		default_asset->init_streams();
	}

// Process the total length in fragments
	ArrayList<MenuEffectPacket*> packets;
	if(!result)
	{
		Label *current_label = mwindow->edl->labels->first;
		mwindow->stop_brender();

		int current_number;
		int number_start;
		int total_digits;
		Render::get_starting_number(default_asset->path, 
			current_number,
			number_start, 
			total_digits);

// Construct all packets for single overwrite confirmation
		for(ptstime fragment_start = total_start, fragment_end;
			fragment_start < total_end;
			fragment_start = fragment_end)
		{
// Get fragment end
			if(strategy & RENDER_FILE_PER_LABEL)
			{
				while(current_label  &&
					current_label->position <= fragment_start)
					current_label = current_label->next;
				if(!current_label)
					fragment_end = total_end;
				else
					fragment_end = current_label->position;
			}
			else
			{
				fragment_end = total_end;
			}

// Get path
			char path[BCTEXTLEN];
			if(strategy & RENDER_FILE_PER_LABEL)
				Render::create_filename(path, 
					default_asset->path, 
					current_number,
					total_digits,
					number_start);
			else
				strcpy(path, default_asset->path);
			current_number++;

			MenuEffectPacket *packet = new MenuEffectPacket(path, 
				fragment_start,
				fragment_end);
			packets.append(packet);
		}

// Test existence of files
		ArrayList<char*> paths;
		for(int i = 0; i < packets.total; i++)
		{
			paths.append(packets.values[i]->path);
		}
		result = ConfirmSave::test_files(mwindow, &paths);
		paths.remove_all();
	}

	for(int current_packet = 0; 
		current_packet < packets.total && !result; 
		current_packet++)
	{
		Asset *asset = new Asset(*default_asset);
		MenuEffectPacket *packet = packets.values[current_packet];
		ptstime fragment_start = packet->start;
		ptstime fragment_end = packet->end;
		strcpy(asset->path, packet->path);

		char *path = new char[strlen(asset->path) + 16];
		strcpy(path, asset->path);
		path_list.append(path);

		File *file = new File;

// Open the output file after getting the information because the sample rate
// is needed here.
		if(!result)
		{
// open output file in write mode
			file->set_processors(mwindow->preferences->processors);

			if(file->open_file(asset, FILE_OPEN_WRITE))
			{
// open failed
				errorbox(_("Couldn't open %s"), asset->path);
				result = 1;
			}
			else
			{
				mwindow->sighandler->push_file(file);
				IndexFile::delete_index(mwindow->preferences, asset);
			}
		}

// run plugins
		if(!result)
		{
			PluginArray *plugin_array;
			plugin_array = create_plugin_array();
			plugin_array->start_plugins(mwindow, 
				mwindow->edl, 
				plugin_server, 
				&plugin_data,
				fragment_start,
				fragment_end,
				file);
			plugin_array->run_plugins();

			plugin_array->stop_plugins();
			mwindow->sighandler->pull_file(file);
			file->close_file();
			delete plugin_array;
		}

		delete file;
		Garbage::delete_object(asset);
	}

	packets.remove_all_objects();

// paste output to tracks
	if(!result && load_mode != LOADMODE_NOTHING)
	{
		if(load_mode == LOADMODE_PASTE)
			mwindow->clear(0);

		mwindow->load_filenames(&path_list, load_mode, 0);
		mwindow->save_backup();
		mwindow->undo->update_undo(title, LOAD_ALL);

		mwindow->restart_brender();
		mwindow->update_plugin_guis();
		mwindow->gui->update(WUPD_SCROLLBARS | WUPD_CANVREDRAW |
			WUPD_TIMEBAR | WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);
		mwindow->sync_parameters(CHANGE_ALL);
	}

	path_list.remove_all_objects();
	Garbage::delete_object(default_asset);
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
}


MenuEffectWindow::MenuEffectWindow(MWindow *mwindow, 
	MenuEffectThread *menueffects, 
	ArrayList<BC_ListBoxItem*> *plugin_list, 
	Asset *asset)
 : BC_Window(MWindow::create_title(N_("Render effect")),
		mwindow->gui->get_abs_cursor_x(1),
		mwindow->gui->get_abs_cursor_y(1) - mwindow->session->menueffect_h / 2,
		mwindow->session->menueffect_w, 
		mwindow->session->menueffect_h, 
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
	this->mwindow = mwindow;
	set_icon(mwindow->theme->get_image("mwindow_icon"));
	mwindow->theme->get_menueffect_sizes(plugin_list ? 1 : 0);

// only add the list if needed
	if(plugin_list)
	{
		add_subwindow(list_title = new BC_Title(mwindow->theme->menueffect_list_x, 
			mwindow->theme->menueffect_list_y, 
			_("Select an effect")));
		add_subwindow(list = new MenuEffectWindowList(this, 
			mwindow->theme->menueffect_list_x, 
			mwindow->theme->menueffect_list_y + list_title->get_h() + 5, 
			mwindow->theme->menueffect_list_w,
			mwindow->theme->menueffect_list_h - list_title->get_h() - 5,
			plugin_list));
	}

	add_subwindow(file_title = new BC_Title(mwindow->theme->menueffect_file_x, 
		mwindow->theme->menueffect_file_y, 
		(char*)((menueffects->strategy & RENDER_FILE_PER_LABEL) ?
			_("Select the first file to render to:") : 
			_("Select a file to render to:"))));

	x = mwindow->theme->menueffect_tools_x;
	y = mwindow->theme->menueffect_tools_y;
	format_tools = new FormatTools(mwindow,
					this, 
					asset,
					x,
					y, 
					asset->audio_data ? SUPPORTS_AUDIO : 0 | asset->video_data ? SUPPORTS_VIDEO : 0,
					0,
					SUPPORTS_VIDEO,
					&menueffects->strategy);

	loadmode = new LoadMode(this,
		x, 
		y, 
		&menueffects->load_mode, 
		1);

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
	mwindow->session->menueffect_w = w;
	mwindow->session->menueffect_h = h;
	mwindow->theme->get_menueffect_sizes(plugin_list ? 1 : 0);

	if(plugin_list)
	{
		list_title->reposition_window(mwindow->theme->menueffect_list_x, 
			mwindow->theme->menueffect_list_y);
		list->reposition_window(mwindow->theme->menueffect_list_x, 
			mwindow->theme->menueffect_list_y + list_title->get_h() + 5, 
			mwindow->theme->menueffect_list_w,
			mwindow->theme->menueffect_list_h - list_title->get_h() - 5);
	}

	file_title->reposition_window(mwindow->theme->menueffect_file_x, 
		mwindow->theme->menueffect_file_y);
	int x = mwindow->theme->menueffect_tools_x;
	int y = mwindow->theme->menueffect_tools_y;
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
}

#define PROMPT_TEXT _("Set up effect panel and hit \"OK\"")

MenuEffectPrompt::MenuEffectPrompt(MWindow *mwindow)
 : BC_Window(MWindow::create_title(N_("Effect Prompt")),
		mwindow->gui->get_abs_cursor_x(1) - 260 / 2,
		mwindow->gui->get_abs_cursor_y(1) - 300,
		MenuEffectPrompt::calculate_w(mwindow->gui), 
		MenuEffectPrompt::calculate_h(mwindow->gui), 
		MenuEffectPrompt::calculate_w(mwindow->gui),
		MenuEffectPrompt::calculate_h(mwindow->gui),
		0,
		0,
		1)
{
	int x = 10, y = 10;
	BC_Title *title;

	set_icon(mwindow->theme->get_image("mwindow_icon"));
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


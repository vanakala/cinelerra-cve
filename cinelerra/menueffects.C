#include "asset.h"
#include "confirmsave.h"
#include "defaults.h"
#include "edl.h"
#include "edlsession.h"
#include "errorbox.h"
#include "file.h"
#include "formatcheck.h"
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
#include "playbackengine.h"
#include "pluginarray.h"
#include "pluginserver.h"
#include "preferences.h"
#include "render.h"
#include "sighandler.h"
#include "theme.h"
#include "tracks.h"



MenuEffects::MenuEffects(MWindow *mwindow)
 : BC_MenuItem(_("Render effect..."))
{
	this->mwindow = mwindow;
}

MenuEffects::~MenuEffects()
{
}


int MenuEffects::handle_event()
{
	thread->set_title("");
	thread->start();
}





MenuEffectPacket::MenuEffectPacket(char *path, int64_t start, int64_t end)
{
	this->start = start;
	this->end = end;
	strcpy(this->path, path);
}

MenuEffectPacket::~MenuEffectPacket()
{
}






MenuEffectThread::MenuEffectThread(MWindow *mwindow)
{
	this->mwindow = mwindow;
	sprintf(title, "");
}

MenuEffectThread::~MenuEffectThread()
{
}





int MenuEffectThread::set_title(char *title)
{
	strcpy(this->title, title);
}

// for recent effect menu items and running new effects
// prompts for an effect if title is blank
void MenuEffectThread::run()
{
// get stuff from main window
	ArrayList<PluginServer*> *plugindb = mwindow->plugindb;
	Defaults *defaults = mwindow->defaults;
	ArrayList<BC_ListBoxItem*> plugin_list;
	ArrayList<PluginServer*> local_plugindb;
	char string[1024];
	int i;
	int result = 0;
// Default configuration
	Asset default_asset;
// Output
	ArrayList<Asset*> assets;


// check for recordable tracks
	if(!get_recordable_tracks(&default_asset))
	{
		sprintf(string, _("No recordable tracks specified."));
		ErrorBox error(PROGRAM_NAME ": Error");
		error.create_objects(string);
		error.run_window();
		return;
	}

// check for plugins
	if(!plugindb->total)
	{
		sprintf(string, _("No plugins available."));
		ErrorBox error(PROGRAM_NAME ": Error");
		error.create_objects(string);
		error.run_window();
		return;
	}


// get default attributes for output file
// used after completion
	get_derived_attributes(&default_asset, defaults);
//	to_tracks = defaults->get("RENDER_EFFECT_TO_TRACKS", 1);
	load_mode = defaults->get("RENDER_EFFECT_LOADMODE", LOAD_PASTE);
	strategy = defaults->get("RENDER_EFFECT_STRATEGY", SINGLE_PASS);

// get plugin information
	int need_plugin;
	if(!strlen(title)) 
		need_plugin = 1; 
	else 
		need_plugin = 0;

// generate a list of plugins for the window
	if(need_plugin)
	{
		mwindow->create_plugindb(default_asset.audio_data, 
			default_asset.video_data, 
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
	int plugin_number, format_error = 0;

	do
	{
		{
			MenuEffectWindow window(mwindow, 
				this, 
				need_plugin ? &plugin_list : 0, 
				&default_asset);
			window.create_objects();
			result = window.run_window();
			plugin_number = window.result;
		}

		if(!result)
		{
			FormatCheck format_check(&default_asset);
			format_error = format_check.check_format();
		}
	}while(format_error && !result);

// save defaults
	save_derived_attributes(&default_asset, defaults);
	defaults->update("RENDER_EFFECT_LOADMODE", load_mode);
	defaults->update("RENDER_EFFECT_STRATEGY", strategy);
	mwindow->save_defaults();

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
			if(!strcmp(plugindb->values[i]->title, title))
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

	if(!result && !strlen(default_asset.path))
	{
		result = 1;        // no output path given
		ErrorBox error(PROGRAM_NAME ": Error");
		error.create_objects(_("No output file specified."));
		error.run_window();
	}

	if(!result && plugin_number < 0)
	{
		result = 1;        // no output path given
		ErrorBox error(PROGRAM_NAME ": Error");
		error.create_objects(_("No effect selected."));
		error.run_window();
	}

// Configuration for realtime plugins.
	KeyFrame plugin_data;        

// get selection to render
// Range
	double total_start, total_end;

	total_start = mwindow->edl->local_session->get_selectionstart();


	if(mwindow->edl->local_session->get_selectionend() == 
		mwindow->edl->local_session->get_selectionstart())
		total_end = mwindow->edl->tracks->total_playable_length();
	else
		total_end = mwindow->edl->local_session->get_selectionend();



// get native units for range
	total_start = to_units(total_start, 0);
	total_end = to_units(total_end, 1);



// Trick boundaries in case of a non-realtime synthesis plugin
	if(plugin && 
		!plugin->realtime && 
		total_end == total_start) total_end = total_start + 1;

// Units are now in the track's units.
	int64_t total_length = (int64_t)total_end - (int64_t)total_start;
// length of output file
	int64_t output_start, output_end;        

	if(!result && total_length <= 0)
	{
		result = 1;        // no output path given
		ErrorBox error(PROGRAM_NAME ": Error");
		error.create_objects(_("No selected range to process."));
		error.run_window();
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
			prompt.create_objects();
			char title[BCTEXTLEN];
			sprintf(title, PROGRAM_NAME ": %s", plugin->title);

// Open the plugin GUI
			plugin->set_mwindow(mwindow);
			plugin->set_keyframe(&plugin_data);
			plugin->set_prompt(&prompt);
			plugin->open_plugin(0, mwindow->preferences, mwindow->edl, 0, -1);
// Must set parameters since there is no plugin object to draw from.
			plugin->get_parameters((int64_t)total_start,
				(int64_t)total_end,
				1);
			plugin->show_gui();

// wait for user input
			result = prompt.run_window();

// Close plugin.
			plugin->save_data(&plugin_data);
			delete plugin;
			default_asset.sample_rate = mwindow->edl->session->sample_rate;
			default_asset.frame_rate = mwindow->edl->session->frame_rate;
			realtime = 1;
		}
		else
// ============================non realtime plugin 
		{
			plugin->set_mwindow(mwindow);
			plugin->open_plugin(0, mwindow->preferences, mwindow->edl, 0, -1);
			result = plugin->get_parameters((int64_t)total_start, 
				(int64_t)total_end, 
				get_recordable_tracks(&default_asset));
// some plugins can change the sample rate and the frame rate


			if(!result)
			{
				default_asset.sample_rate = plugin->get_samplerate();
				default_asset.frame_rate = plugin->get_framerate();
			}
			delete plugin;
			realtime = 0;
		}

// Should take from first recordable track
		default_asset.width = mwindow->edl->session->output_w;
		default_asset.height = mwindow->edl->session->output_h;
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
		Render::get_starting_number(default_asset.path, 
			current_number,
			number_start, 
			total_digits);



// Construct all packets for single overwrite confirmation
		for(int64_t fragment_start = (int64_t)total_start, fragment_end;
			fragment_start < (int64_t)total_end;
			fragment_start = fragment_end)
		{
// Get fragment end
			if(strategy == FILE_PER_LABEL || strategy == FILE_PER_LABEL_FARM)
			{
				while(current_label  &&
					to_units(current_label->position, 0) <= fragment_start)
					current_label = current_label->next;
				if(!current_label)
					fragment_end = (int64_t)total_end;
				else
					fragment_end = to_units(current_label->position, 0);
			}
			else
			{
				fragment_end = (int64_t)total_end;
			}

// Get path
			char path[BCTEXTLEN];
			if(strategy == FILE_PER_LABEL || strategy == FILE_PER_LABEL_FARM) 
				Render::create_filename(path, 
					default_asset.path, 
					current_number,
					total_digits,
					number_start);
			else
				strcpy(path, default_asset.path);
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
		Asset *asset = new Asset(default_asset);
		MenuEffectPacket *packet = packets.values[current_packet];
		int64_t fragment_start = packet->start;
		int64_t fragment_end = packet->end;
		strcpy(asset->path, packet->path);

		assets.append(asset);
		File *file = new File;

// Open the output file after getting the information because the sample rate
// is needed here.
		if(!result)
		{
// open output file in write mode
			file->set_processors(mwindow->preferences->processors);
			if(file->open_file(mwindow->plugindb, 
				asset, 
				0, 
				1, 
				mwindow->edl->session->sample_rate, 
				mwindow->edl->session->frame_rate))
			{
// open failed
				sprintf(string, _("Couldn't open %s"), asset->path);
				ErrorBox error(PROGRAM_NAME ": Error");
				error.create_objects(string);
				error.run_window();
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
// position file
			output_start = 0;

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
			asset->audio_length = file->asset->audio_length;
			asset->video_length = file->asset->video_length;
			delete plugin_array;
		}

		delete file;
	}

	packets.remove_all_objects();

// paste output to tracks
	if(!result && load_mode != LOAD_NOTHING)
	{
		mwindow->gui->lock_window();
		mwindow->undo->update_undo_before(title, LOAD_ALL);

		if(load_mode == LOAD_PASTE)
			mwindow->clear(0);
		mwindow->load_assets(&assets,
			-1,
			load_mode,
			0,
			0,
			mwindow->edl->session->labels_follow_edits, 
			mwindow->edl->session->plugins_follow_edits);


		mwindow->save_backup();
		mwindow->undo->update_undo_after();



		mwindow->restart_brender();
		mwindow->update_plugin_guis();
		mwindow->gui->update(1, 
			2,
			1,
			1,
			1,
			1,
			0);
		mwindow->sync_parameters(CHANGE_ALL);
		mwindow->gui->unlock_window();
	}

	assets.remove_all_objects();
}




MenuEffectItem::MenuEffectItem(MenuEffects *menueffect, char *string)
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
 : BC_Window(PROGRAM_NAME ": Render effect", 
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
	this->menueffects = menueffects; 
	this->plugin_list = plugin_list; 
	this->asset = asset;
	this->mwindow = mwindow;
}

MenuEffectWindow::~MenuEffectWindow()
{
	delete format_tools;
}
// 
// int MenuEffectWindow::calculate_w(int use_plugin_list)
// {
// 	return use_plugin_list ? 580 : 420;
// }
// 
// int MenuEffectWindow::calculate_h(int use_plugin_list)
// {
// 	return 350;
// }


int MenuEffectWindow::create_objects()
{
	int x, y;
	result = -1;
	mwindow->theme->get_menueffect_sizes(plugin_list ? 1 : 0);

// only add the list if needed
	if(plugin_list)
	{
		add_subwindow(list_title = new BC_Title(mwindow->theme->menueffect_list_x, 
			mwindow->theme->menueffect_list_y, 
			_("Select an effect")));
		add_subwindow(list = new MenuEffectWindowList(this, 
			mwindow->theme->menueffect_list_x, 
			mwindow->theme->menueffect_list_y + 20, 
			mwindow->theme->menueffect_list_w,
			mwindow->theme->menueffect_list_h,
			plugin_list));
	}

	add_subwindow(file_title = new BC_Title(mwindow->theme->menueffect_file_x, 
		mwindow->theme->menueffect_file_y, 
		(char*)((menueffects->strategy == FILE_PER_LABEL  || menueffects->strategy == FILE_PER_LABEL_FARM) ? 
			_("Select the first file to render to:") : 
			_("Select a file to render to:"))));

	x = mwindow->theme->menueffect_tools_x;
	y = mwindow->theme->menueffect_tools_y;
	format_tools = new FormatTools(mwindow,
					this, 
					asset);
	format_tools->create_objects(x, 
					y, 
					asset->audio_data, 
					asset->video_data, 
					0, 
					0, 
					0,
					1,
					0,
					0,
					&menueffects->strategy,
					0);

	loadmode = new LoadMode(mwindow, 
		this, 
		x, 
		y, 
		&menueffects->load_mode, 
		1);
	loadmode->create_objects();

	add_subwindow(new MenuEffectWindowOK(this));
	add_subwindow(new MenuEffectWindowCancel(this));
	show_window();
	flush();
	return 0;
}

int MenuEffectWindow::resize_event(int w, int h)
{
	mwindow->session->menueffect_w = w;
	mwindow->session->menueffect_h = h;
	mwindow->theme->get_menueffect_sizes(plugin_list ? 1 : 0);

	if(plugin_list)
	{
		list_title->reposition_window(mwindow->theme->menueffect_list_x, 
			mwindow->theme->menueffect_list_y);
		list->reposition_window(mwindow->theme->menueffect_list_x, 
			mwindow->theme->menueffect_list_y + 20, 
			mwindow->theme->menueffect_list_w,
			mwindow->theme->menueffect_list_h);
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
	if(get_keypress() == 13) 
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
		LISTBOX_TEXT, 
		plugin_list)
{ 
	this->window = window; 
}

int MenuEffectWindowList::handle_event() 
{
	window->result = get_selection_number(0, 0);
	window->set_done(0); 
}



MenuEffectPrompt::MenuEffectPrompt(MWindow *mwindow)
 : BC_Window(PROGRAM_NAME ": Effect Prompt", 
 		mwindow->gui->get_abs_cursor_x(1) - 260 / 2,
		mwindow->gui->get_abs_cursor_y(1) - 300,
 		260, 
		80, 
		260,
		80,
		0,
		0,
		1)
{
}

MenuEffectPrompt::~MenuEffectPrompt()
{
}

int MenuEffectPrompt::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, _("Set up effect panel and hit \"OK\"")));
	y += 20;
	add_subwindow(new BC_OKButton(this));
	x = get_w() - 100;
	add_subwindow(new BC_CancelButton(this));
	show_window();
	raise_window();
	flush();
	return 0;
}


#include "assets.h"
#include "defaults.h"
#include "edl.h"
#include "edlsession.h"
#include "errorbox.h"
#include "file.h"
#include "formatcheck.h"
#include "indexfile.h"
#include "keyframe.h"
#include "keys.h"
#include "loadmode.h"
#include "localsession.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "menueffects.h"
#include "neworappend.h"
#include "playbackengine.h"
#include "pluginarray.h"
#include "pluginserver.h"
#include "preferences.h"
#include "render.h"
#include "tracks.h"

MenuEffects::MenuEffects(MWindow *mwindow)
 : BC_MenuItem("Render effect...")
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





MenuEffectPacket::MenuEffectPacket()
{
	start = end = 0.0;
	path[0] = 0;
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

//printf("MenuEffectThread::run 1\n");
//	sprintf(string, "");
//	defaults->get("EFFECTPATH", string);

// check for recordable tracks
	if(!get_recordable_tracks(&default_asset))
	{
		sprintf(string, "No recorable tracks specified.");
		ErrorBox error(PROGRAM_NAME ": Error");
		error.create_objects(string);
		error.run_window();
		return;
	}

//printf("MenuEffectThread::run 1\n");
// check for plugins
	if(!plugindb->total)
	{
		sprintf(string, "No plugins available.");
		ErrorBox error(PROGRAM_NAME ": Error");
		error.create_objects(string);
		error.run_window();
		return;
	}

//printf("MenuEffectThread::run 1\n");

//printf("MenuEffectThread::run 1\n");
// get default attributes for output file
// used after completion
	get_derived_attributes(&default_asset, defaults);
//	to_tracks = defaults->get("RENDER_EFFECT_TO_TRACKS", 1);
	load_mode = defaults->get("RENDER_EFFECT_LOADMODE", LOAD_PASTE);
	strategy = defaults->get("RENDER_EFFECT_STRATEGY", SINGLE_PASS);

//printf("MenuEffectThread::run 1\n");
// get plugin information
	int need_plugin;
	if(!strlen(title)) 
		need_plugin = 1; 
	else 
		need_plugin = 0;

//printf("MenuEffectThread::run 1\n");
// generate a list of plugins for the window
	if(need_plugin)
	{
		mwindow->create_plugindb(default_asset.audio_data, 
			default_asset.video_data, 
			-1, 
			0,
			0,
			local_plugindb);

//printf("MenuEffectThread::run 1\n");
		for(int i = 0; i < local_plugindb.total; i++)
		{
			plugin_list.append(new BC_ListBoxItem(local_plugindb.values[i]->title, BLACK));
		}
	}

//printf("MenuEffectThread::run 2\n");
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

//printf("MenuEffectThread::run 3\n");
		if(!result)
		{
			FormatCheck format_check(&default_asset);
			format_error = format_check.check_format();
		}
	}while(format_error && !result);

//printf("MenuEffectThread::run 4\n");
// save defaults
	save_derived_attributes(&default_asset, defaults);
	defaults->update("RENDER_EFFECT_LOADMODE", load_mode);
	defaults->update("RENDER_EFFECT_STRATEGY", strategy);
	mwindow->save_defaults();

//printf("MenuEffectThread::run 5\n");
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

//printf("MenuEffectThread::run 6\n");
	if(!result && !strlen(default_asset.path))
	{
		result = 1;        // no output path given
		ErrorBox error(PROGRAM_NAME ": Error");
		error.create_objects("No output file specified.");
		error.run_window();
	}

	if(!result && plugin_number < 0)
	{
		result = 1;        // no output path given
		ErrorBox error(PROGRAM_NAME ": Error");
		error.create_objects("No effect selected.");
		error.run_window();
	}
//printf("MenuEffectThread::run 7\n");

// Configuration for realtime plugins.
	KeyFrame plugin_data;        
//printf("MenuEffectThread::run 8\n");

// get selection to render
// Range
	double total_start, total_end;

//printf("MenuEffectThread::run 9\n");
	total_start = mwindow->edl->local_session->get_selectionstart();


	if(mwindow->edl->local_session->get_selectionend() == 
		mwindow->edl->local_session->get_selectionstart())
		total_end = mwindow->edl->tracks->total_playable_length();
	else
		total_end = mwindow->edl->local_session->get_selectionend();



//printf("MenuEffectThread::run 10\n");
// get native units for range
	total_start = to_units(total_start, 0);
	total_end = to_units(total_end, 1);



// Trick boundaries in case of a non-realtime synthesis plugin
	if(plugin && 
		!plugin->realtime && 
		total_end == total_start) total_end = total_start + 1;
//printf("MenuEffectThread::run 11\n");

//printf("MenuEffectThread::run 1\n");
// Units are now in the track's units.
	long total_length = (long)total_end - (long)total_start;
// length of output file
	long output_start, output_end;        

	if(!result && total_length <= 0)
	{
		result = 1;        // no output path given
		ErrorBox error(PROGRAM_NAME ": Error");
		error.create_objects("No selected range to process.");
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
//printf("MenuEffectThread::run 10 %s\n", plugin->title);
			MenuEffectPrompt prompt(mwindow);
			prompt.create_objects();
			char title[1024];
			sprintf(title, PROGRAM_NAME ": %s", plugin->title);
//printf("MenuEffectThread::run 11 %s\n", plugin->title);

// Open the plugin GUI
			plugin->set_mwindow(mwindow);
			plugin->set_keyframe(&plugin_data);
			plugin->set_prompt(&prompt);
//printf("MenuEffectThread::run 11 %s\n", plugin->title);
			plugin->open_plugin(0, mwindow->edl, 0);
//printf("MenuEffectThread::run 11 %s\n", plugin->title);
			plugin->show_gui();
//printf("MenuEffectThread::run 12 %s\n", plugin->title);

// wait for user input
			result = prompt.run_window();
//printf("MenuEffectThread::run 13 %s\n", plugin->title);

// Close plugin.
			plugin->save_data(&plugin_data);
//printf("MenuEffectThread::run 13 %s\n", plugin->title);
			delete plugin;
//printf("MenuEffectThread::run 14\n");
			default_asset.sample_rate = mwindow->edl->session->sample_rate;
			default_asset.frame_rate = mwindow->edl->session->frame_rate;
			realtime = 1;
		}
		else
// ============================non realtime plugin 
		{
//printf("MenuEffectThread::run 15\n");
			plugin->set_mwindow(mwindow);
//printf("MenuEffectThread::run 16\n");
			plugin->open_plugin(0, mwindow->edl, 0);
//printf("MenuEffectThread::run 17\n");
			result = plugin->get_parameters();
// some plugins can change the sample rate and the frame rate
//printf("MenuEffectThread::run 18\n");


			if(!result)
			{
				default_asset.sample_rate = plugin->get_samplerate();
				default_asset.frame_rate = plugin->get_framerate();
			}
//printf("MenuEffectThread::run 19\n");
			delete plugin;
			realtime = 0;
		}

// Should take from first recordable track
		default_asset.width = mwindow->edl->session->output_w;
		default_asset.height = mwindow->edl->session->output_h;
	}

//printf("MenuEffectThread::run 20\n");
// Process the total length in fragments
	Label *current_label = mwindow->edl->labels->first;
	mwindow->stop_brender();

	int current_number;
	int number_start;
	int total_digits;
	Render::get_starting_number(default_asset.path, 
		current_number,
		number_start, 
		total_digits);
	for(long fragment_start = (long)total_start, fragment_end; 
		fragment_start < (long)total_end && !result; 
		fragment_start = fragment_end)
	{
// Get fragment end
		if(strategy == FILE_PER_LABEL || strategy == FILE_PER_LABEL_FARM)
		{
			while(current_label && 
				to_units(current_label->position, 0) <= fragment_start)
					current_label = current_label->next;

			if(!current_label)
				fragment_end = (long)total_end;
			else
				fragment_end = to_units(current_label->position, 0);
		}
		else
		{
			fragment_end = (long)total_end;
		}

// Create asset
		Asset *asset = new Asset(default_asset);
		if(strategy == FILE_PER_LABEL || strategy == FILE_PER_LABEL_FARM) 
			Render::create_filename(asset->path, 
				default_asset.path, 
				current_number,
				total_digits,
				number_start);
		current_number++;

		result = Render::test_existence(mwindow, asset);

		if(!result)
		{
			assets.append(asset);
			File *file = new File;

// Open the output file after getting the information because the sample rate
// is needed here.
			if(!result)
			{
// open output file in write mode
				file->set_processors(mwindow->edl->session->smp + 1);
				if(file->open_file(mwindow->plugindb, 
					asset, 
					0, 
					1, 
					mwindow->edl->session->sample_rate, 
					mwindow->edl->session->frame_rate))
				{
// open failed
					sprintf(string, "Couldn't open %s", asset->path);
					ErrorBox error(PROGRAM_NAME ": Error");
					error.create_objects(string);
					error.run_window();
					result = 1;
				}
				else
					IndexFile::delete_index(mwindow->preferences, asset);
			}

//printf("MenuEffectThread::run 10\n");
// run plugins
			if(!result)
			{
// position file
				output_start = 0;

				PluginArray *plugin_array;
				plugin_array = create_plugin_array();

//printf("MenuEffectThread::run 11\n");
				plugin_array->start_plugins(mwindow, 
					mwindow->edl, 
					plugin_server, 
					&plugin_data,
					fragment_start,
					fragment_end,
					file);
//printf("MenuEffectThread::run 12\n");
				plugin_array->run_plugins();

//printf("MenuEffectThread::run 13\n");
				plugin_array->stop_plugins();
				file->close_file();
				asset->audio_length = file->asset->audio_length;
				asset->video_length = file->asset->video_length;
//printf("MenuEffectThread::run 14 %d %d\n", asset->audio_length, asset->video_length);
				delete plugin_array;
//printf("MenuEffectThread::run 16\n");
			}

			delete file;
		}
	}
printf("MenuEffectThread::run 16 %d\n", result);


// paste output to tracks
	if(!result && load_mode != LOAD_NOTHING)
	{
		mwindow->gui->lock_window();
//printf("MenuEffectThread::run 17\n");
		mwindow->undo->update_undo_before(title, LOAD_ALL);

//printf("MenuEffectThread::run 18\n");
		mwindow->load_assets(&assets,
			-1,
			load_mode,
			0,
			0,
			mwindow->edl->session->labels_follow_edits, 
			mwindow->edl->session->plugins_follow_edits);

//printf("MenuEffectThread::run 19\n");

		mwindow->save_backup();
		mwindow->undo->update_undo_after();



		mwindow->restart_brender();
		mwindow->update_plugin_guis();
		mwindow->gui->update(1, 
			2,
			1,
			1,
			0,
			1,
			0);
		mwindow->sync_parameters(CHANGE_ALL);
//printf("MenuEffectThread::run 22\n");
		mwindow->gui->unlock_window();
//printf("MenuEffectThread::run 23\n");
	}

	assets.remove_all_objects();
//printf("MenuEffectThread::run 24\n");
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
 		mwindow->gui->get_abs_cursor_x(),
		mwindow->gui->get_abs_cursor_y() - calculate_h((int)plugin_list) / 2,
		calculate_w((int)plugin_list), calculate_h((int)plugin_list), 
		calculate_w((int)plugin_list), calculate_h((int)plugin_list),
		0,
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

int MenuEffectWindow::calculate_w(int use_plugin_list)
{
	return use_plugin_list ? 580 : 420;
}

int MenuEffectWindow::calculate_h(int use_plugin_list)
{
	return 350;
}


int MenuEffectWindow::create_objects()
{
	int x, y;
	result = -1;

	x = 10;
	y = 5;
//printf("MenuEffectWindow::create_objects 1\n");
// only add the list if needed
	if(plugin_list)
	{
		add_subwindow(new BC_Title(x, y, "Select an effect"));
		add_subwindow(list = new MenuEffectWindowList(this, x, plugin_list));
		x += 180;
		y += 30;
	}
//printf("MenuEffectWindow::create_objects 1\n");

	add_subwindow(new BC_Title(x, 
		y, 
		(char*)((menueffects->strategy == FILE_PER_LABEL  || menueffects->strategy == FILE_PER_LABEL_FARM) ? 
			"Select the first file to render to:" : 
			"Select a file to render to:")));
//printf("MenuEffectWindow::create_objects 1\n");

	y += 20;
	format_tools = new FormatTools(mwindow,
					this, 
					asset);
//printf("MenuEffectWindow::create_objects 1\n");
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

	loadmode = new LoadMode(mwindow, this, x, y, &menueffects->load_mode, 1);
	loadmode->create_objects();

	add_subwindow(new MenuEffectWindowOK(this));
	add_subwindow(new MenuEffectWindowCancel(this));
//printf("MenuEffectWindow::create_objects 1\n");
	show_window();
	flush();
//printf("MenuEffectWindow::create_objects 2\n");
	return 0;
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

MenuEffectWindowList::MenuEffectWindowList(MenuEffectWindow *window, int x, ArrayList<BC_ListBoxItem*> *plugin_list)
 : BC_ListBox(x, 
 		30, 
		170, 
		270, 
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
 		mwindow->gui->get_abs_cursor_x() - 260 / 2,
		mwindow->gui->get_abs_cursor_y() - 300,
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
	add_subwindow(new BC_Title(x, y, "Set up effect panel and hit \"OK\""));
	y += 20;
	add_subwindow(new BC_OKButton(this));
	x = get_w() - 100;
	add_subwindow(new BC_CancelButton(this));
	show_window();
	raise_window();
	flush();
	return 0;
}


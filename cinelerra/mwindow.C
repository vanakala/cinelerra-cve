#include "asset.h"
#include "assets.h"
#include "awindowgui.h"
#include "awindow.h"
#include "batchrender.h"
#include "bcdisplayinfo.h"
#include "brender.h"
#include "cache.h"
#include "channel.h"
#include "channeldb.h"
#include "clip.h"
#include "colormodels.h"
#include "cplayback.h"
#include "ctimebar.h"
#include "cwindowgui.h"
#include "cwindow.h"
#include "defaults.h"
#include "editpanel.h"
#include "edl.h"
#include "edlsession.h"
#include "errorbox.h"
#include "fileformat.h"
#include "file.h"
#include "filesystem.h"
#include "filexml.h"
#include "indexfile.h"
#include "language.h"
#include "levelwindowgui.h"
#include "levelwindow.h"
#include "loadfile.inc"
#include "localsession.h"
#include "maincursor.h"
#include "mainindexes.h"
#include "mainmenu.h"
#include "mainprogress.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mbuttons.h"
#include "mutex.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "new.h"
#include "patchbay.h"
#include "playbackengine.h"
#include "plugin.h"
#include "pluginserver.h"
#include "pluginset.h"
#include "preferences.h"
#include "record.h"
#include "recordlabel.h"
#include "render.h"
#include "samplescroll.h"
#include "sighandler.h"
#include "splashgui.h"
#include "statusbar.h"
#include "theme.h"
#include "threadloader.h"
#include "timebar.h"
#include "trackcanvas.h"
#include "track.h"
#include "tracking.h"
#include "trackscroll.h"
#include "tracks.h"
#include "transportque.h"
#include "vframe.h"
#include "videodevice.inc"
#include "videowindow.h"
#include "vplayback.h"
#include "vwindowgui.h"
#include "vwindow.h"
#include "zoombar.h"

#include <string.h>



extern "C"
{




// Hack for libdv to remove glib dependancy

// void
// g_log (const char    *log_domain,
//        int  log_level,
//        const char    *format,
//        ...)
// {
// }
// 
// void
// g_logv (const char    *log_domain,
//        int  log_level,
//        const char    *format,
//        ...)
// {
// }
// 


// Hack for XFree86 4.1.0

int atexit(void (*function)(void))
{
	return 0;
}



}





MWindow::MWindow()
{
	plugin_gui_lock = new Mutex("MWindow::plugin_gui_lock");
	brender_lock = new Mutex("MWindow::brender_lock");
	brender = 0;
	session = 0;
	channeldb_buz = new ChannelDB;
	channeldb_v4l2jpeg = new ChannelDB;
}

MWindow::~MWindow()
{
	brender_lock->lock("MWindow::~MWindow");
	if(brender) delete brender;
	brender = 0;
	brender_lock->unlock();
	delete brender_lock;

	delete mainindexes;

TRACE("MWindow::~MWindow 1\n");
	clean_indexes();
TRACE("MWindow::~MWindow 2\n");

	save_defaults();
TRACE("MWindow::~MWindow 3\n");
// Give up and go to a movie
	exit(0);

TRACE("MWindow::~MWindow 4\n");
	delete mainprogress;
	delete audio_cache;             // delete the cache after the assets
	delete video_cache;             // delete the cache after the assets
	if(gui) delete gui;
	delete undo;
	delete preferences;
	delete defaults;
	delete render;
//	delete renderlist;
	delete awindow;
	delete vwindow;
	delete cwindow;
	delete lwindow;
	plugin_guis->remove_all_objects();
	delete plugin_guis;
	delete plugin_gui_lock;
}

void MWindow::init_defaults(Defaults* &defaults, char *config_path)
{
	char path[BCTEXTLEN];
// Use user supplied path
	if(config_path[0])
	{
		strcpy(path, config_path);
	}
	else
	{
// set the .bcast path
		FileSystem fs;

		sprintf(path, "%s", BCASTDIR);
		fs.complete_path(path);
		if(fs.is_dir(path)) 
		{
			fs.create_dir(path); 
		}

// load the defaults
		strcat(path, "Cinelerra_rc");
	}

	defaults = new Defaults(path);
	defaults->load();
}

void MWindow::init_plugin_path(Preferences *preferences, 
	ArrayList<PluginServer*>* &plugindb,
	FileSystem *fs,
	SplashGUI *splash_window,
	int *counter)
{
	int result = 0;
	PluginServer *newplugin;

	if(!result)
	{
		for(int i = 0; i < fs->dir_list.total; i++)
		{
			char path[BCTEXTLEN];
			strcpy(path, fs->dir_list.values[i]->path);

// File is a directory
			if(!fs->is_dir(path))
			{
				continue;
			}
			else
			{
// Try to query the plugin
				fs->complete_path(path);
//printf("MWindow::init_plugin_path %s\n", path);
				PluginServer *new_plugin = new PluginServer(path);
				int result = new_plugin->open_plugin(1, preferences, 0, 0, -1);

				if(!result)
				{
					plugindb->append(new_plugin);
					new_plugin->close_plugin();
					if(splash_window)
						splash_window->operation->update(_(new_plugin->title));
				}
				else
				if(result == PLUGINSERVER_IS_LAD)
				{
					delete new_plugin;
// Open LAD subplugins
					int id = 0;
					do
					{
						new_plugin = new PluginServer(path);
						result = new_plugin->open_plugin(1,
							preferences,
							0,
							0,
							id);
						id++;
						if(!result)
						{
							plugindb->append(new_plugin);
							new_plugin->close_plugin();
							if(splash_window)
								splash_window->operation->update(_(new_plugin->title));
						}
					}while(!result);
				}
				else
				{
// Plugin failed to open
					delete new_plugin;
				}
			}
			if(splash_window) splash_window->progress->update((*counter)++);
		}
	}
}

void MWindow::init_plugins(Preferences *preferences, 
	ArrayList<PluginServer*>* &plugindb,
	SplashGUI *splash_window)
{
	plugindb = new ArrayList<PluginServer*>;



	FileSystem cinelerra_fs;
	ArrayList<FileSystem*> lad_fs;
	int result = 0;

// Get directories
	cinelerra_fs.set_filter("[*.plugin][*.so]");
	result = cinelerra_fs.update(preferences->global_plugin_dir);

	if(result)
	{
		fprintf(stderr, 
			_("MWindow::init_plugins: couldn't open %s directory\n"),
			preferences->global_plugin_dir);
	}

// Parse LAD environment variable
	char *env = getenv("LADSPA_PATH");
	if(env)
	{
		char string[BCTEXTLEN];
		char *ptr1 = env;
		while(ptr1)
		{
			char *ptr = strchr(ptr1, ':');
			char *end;
			if(ptr)
			{
				end = ptr;
			}
			else
			{
				end = env + strlen(env);
			}

			if(end > ptr1)
			{
				int len = end - ptr1;
				memcpy(string, ptr1, len);
				string[len] = 0;


				FileSystem *fs = new FileSystem;
				lad_fs.append(fs);
				fs->set_filter("*.so");
				result = fs->update(string);

				if(result)
				{
					fprintf(stderr, 
						_("MWindow::init_plugins: couldn't open %s directory\n"),
						string);
				}
			}

			if(ptr)
				ptr1 = ptr + 1;
			else
				ptr1 = ptr;
		};
	}

	int total = cinelerra_fs.total_files();
	int counter = 0;
	for(int i = 0; i < lad_fs.total; i++)
		total += lad_fs.values[i]->total_files();
	if(splash_window) splash_window->progress->update_length(total);

// Cinelerra
	init_plugin_path(preferences,
		plugindb,
		&cinelerra_fs,
		splash_window,
		&counter);

// LAD
	for(int i = 0; i < lad_fs.total; i++)
		init_plugin_path(preferences,
			plugindb,
			lad_fs.values[i],
			splash_window,
			&counter);

	lad_fs.remove_all_objects();
}

void MWindow::delete_plugins()
{
	for(int i = 0; i < plugindb->total; i++)
	{
		delete plugindb->values[i];
	}
	delete plugindb;
}

void MWindow::create_plugindb(int do_audio, 
		int do_video, 
		int is_realtime, 
		int is_transition,
		int is_theme,
		ArrayList<PluginServer*> &plugindb)
{
// Get plugins
	for(int i = 0; i < this->plugindb->total; i++)
	{
		PluginServer *current = this->plugindb->values[i];

		if(current->audio == do_audio &&
			current->video == do_video &&
			(current->realtime == is_realtime || is_realtime < 0) &&
			current->transition == is_transition &&
			current->theme == is_theme)
			plugindb.append(current);
	}

// Alphabetize list by title
	int done = 0;
	while(!done)
	{
		done = 1;
		
		for(int i = 0; i < plugindb.total - 1; i++)
		{
			PluginServer *value1 = plugindb.values[i];
			PluginServer *value2 = plugindb.values[i + 1];
			if(strcmp(value1->title, value2->title) > 0)
			{
				done = 0;
				plugindb.values[i] = value2;
				plugindb.values[i + 1] = value1;
			}
		}
	}
}

PluginServer* MWindow::scan_plugindb(char *title,
		int data_type)
{
	if(data_type < 0)
	{
		printf("MWindow::scan_plugindb data_type < 0\n");
		return 0;
	}

	for(int i = 0; i < plugindb->total; i++)
	{
		PluginServer *server = plugindb->values[i];
		if(!strcasecmp(server->title, title) &&
		((data_type == TRACK_AUDIO && server->audio) ||
		(data_type == TRACK_VIDEO && server->video))) 
			return plugindb->values[i];
	}
	return 0;
}

void MWindow::init_preferences()
{
	preferences = new Preferences;
	preferences->load_defaults(defaults);
	session = new MainSession(this);
	session->load_defaults(defaults);
}

void MWindow::clean_indexes()
{
	FileSystem fs;
	int total_excess;
	long oldest;
	int oldest_item;
	int result;
	char string[BCTEXTLEN];

// Delete extra indexes
	fs.set_filter("*.idx");
	fs.complete_path(preferences->index_directory);
	fs.update(preferences->index_directory);
//printf("MWindow::clean_indexes 1 %d\n", fs.dir_list.total);

// Eliminate directories
	result = 1;
	while(result)
	{
		result = 0;
		for(int i = 0; i < fs.dir_list.total && !result; i++)
		{
			fs.join_names(string, preferences->index_directory, fs.dir_list.values[i]->name);
			if(!fs.is_dir(string))
			{
				delete fs.dir_list.values[i];
				fs.dir_list.remove_number(i);
				result = 1;
			}
		}
	}
	total_excess = fs.dir_list.total - preferences->index_count;

//printf("MWindow::clean_indexes 2 %d\n", fs.dir_list.total);
	while(total_excess > 0)
	{
// Get oldest
		for(int i = 0; i < fs.dir_list.total; i++)
		{
			fs.join_names(string, preferences->index_directory, fs.dir_list.values[i]->name);

			if(i == 0 || fs.get_date(string) <= oldest)
			{
				oldest = fs.get_date(string);
				oldest_item = i;
			}
		}

		fs.join_names(string, preferences->index_directory, fs.dir_list.values[oldest_item]->name);
		if(remove(string))
			perror("delete_indexes");
		delete fs.dir_list.values[oldest_item];
		fs.dir_list.remove_number(oldest_item);
		total_excess--;
	}
}

void MWindow::init_awindow()
{
	awindow = new AWindow(this);
	awindow->create_objects();
}

void MWindow::init_theme()
{
	theme = 0;

	for(int i = 0; i < plugindb->total; i++)
	{
		if(plugindb->values[i]->theme &&
			!strcasecmp(preferences->theme, plugindb->values[i]->title))
		{
			PluginServer plugin = *plugindb->values[i];
			plugin.open_plugin(0, preferences, 0, 0, -1);
			theme = plugin.new_theme();
			theme->mwindow = this;
			strcpy(theme->path, plugin.path);
			plugin.close_plugin();
		}
	}

	if(!theme)
	{
		fprintf(stderr, _("MWindow::init_theme: theme %s not found.\n"), preferences->theme);
		exit(1);
	}

// Load user images
	theme->initialize();
// Load images which may have been forgotten
	theme->Theme::initialize();

	theme->check_used();
}

void MWindow::init_edl()
{
	edl = new EDL;
	edl->create_objects();
    edl->load_defaults(defaults);
	edl->create_default_tracks();
	edl->tracks->update_y_pixels(theme);
}

void MWindow::init_compositor()
{
	cwindow = new CWindow(this);
    cwindow->create_objects();
}

void MWindow::init_levelwindow()
{
	lwindow = new LevelWindow(this);
	lwindow->create_objects();
}

void MWindow::init_viewer()
{
	vwindow = new VWindow(this);
	vwindow->load_defaults();
	vwindow->create_objects();
}

void MWindow::init_cache()
{
	audio_cache = new CICache(edl, preferences, plugindb);
	video_cache = new CICache(edl, preferences, plugindb);
}

void MWindow::init_channeldb()
{
	channeldb_buz->load("channeldb_buz");
	channeldb_v4l2jpeg->load("channeldb_v4l2jpeg");
}

void MWindow::init_menus()
{
	char string[BCTEXTLEN];
	cmodel_to_text(string, BC_RGB888);
	colormodels.append(new ColormodelItem(string, BC_RGB888));
	cmodel_to_text(string, BC_RGBA8888);
	colormodels.append(new ColormodelItem(string, BC_RGBA8888));
	cmodel_to_text(string, BC_RGB161616);
	colormodels.append(new ColormodelItem(string, BC_RGB161616));
	cmodel_to_text(string, BC_RGBA16161616);
	colormodels.append(new ColormodelItem(string, BC_RGBA16161616));
	cmodel_to_text(string, BC_YUV888);
	colormodels.append(new ColormodelItem(string, BC_YUV888));
	cmodel_to_text(string, BC_YUVA8888);
	colormodels.append(new ColormodelItem(string, BC_YUVA8888));
	cmodel_to_text(string, BC_YUV161616);
	colormodels.append(new ColormodelItem(string, BC_YUV161616));
	cmodel_to_text(string, BC_YUVA16161616);
	colormodels.append(new ColormodelItem(string, BC_YUVA16161616));
}

void MWindow::init_indexes()
{
	mainindexes = new MainIndexes(this);
	mainindexes->start_loop();
}

void MWindow::init_gui()
{
	gui = new MWindowGUI(this);
	gui->create_objects();
	gui->load_defaults(defaults);
}

void MWindow::init_signals()
{
	sighandler = new SigHandler;
	sighandler->initialize();
ENABLE_BUFFER
}

void MWindow::init_render()
{
	render = new Render(this);
//	renderlist = new Render(this);
	batch_render = new BatchRenderThread(this);
}

void MWindow::init_brender()
{
	if(preferences->use_brender && !brender)
	{
		brender_lock->lock("MWindow::init_brender 1");
		brender = new BRender(this);
		brender->initialize();
		session->brender_end = 0;
		brender_lock->unlock();
	}
	else
	if(!preferences->use_brender && brender)
	{
		brender_lock->lock("MWindow::init_brender 2");
		delete brender;
		brender = 0;
		session->brender_end = 0;
		brender_lock->unlock();
	}
	if(brender) brender->restart(edl);
}

void MWindow::restart_brender()
{
//printf("MWindow::restart_brender 1\n");
	if(brender) brender->restart(edl);
}

void MWindow::stop_brender()
{
	if(brender) brender->stop();
}

int MWindow::brender_available(int position)
{
	int result = 0;
	brender_lock->lock("MWindow::brender_available 1");
	if(brender)
	{
		if(brender->map_valid)
		{
			brender->map_lock->lock("MWindow::brender_available 2");
			if(position < brender->map_size &&
				position >= 0)
			{
//printf("MWindow::brender_available 1 %d %d\n", position, brender->map[position]);
				if(brender->map[position] == BRender::RENDERED)
					result = 1;
			}
			brender->map_lock->unlock();
		}
	}
	brender_lock->unlock();
	return result;
}

void MWindow::set_brender_start()
{
	edl->session->brender_start = edl->local_session->get_selectionstart();
	restart_brender();
	gui->canvas->draw_overlays();
	gui->canvas->flash();
}



int MWindow::load_filenames(ArrayList<char*> *filenames, int load_mode)
{
TRACE("MWindow::load_filenames 1");
	ArrayList<EDL*> new_edls;
	ArrayList<Asset*> new_assets;
//printf("load_filenames 1\n");

// Need to stop playback since tracking depends on the EDL not getting
// deleted.
	cwindow->playback_engine->que->send_command(STOP,
		CHANGE_NONE, 
		0,
		0);
	vwindow->playback_engine->que->send_command(STOP,
		CHANGE_NONE, 
		0,
		0);
	cwindow->playback_engine->interrupt_playback(0);
	vwindow->playback_engine->interrupt_playback(0);



// Define new_edls and new_assets to load
	int result = 0;
	for(int i = 0; i < filenames->total; i++)
	{
// Get type of file
		File *new_file = new File;
		Asset *new_asset = new Asset(filenames->values[i]);
		EDL *new_edl = new EDL;
		char string[BCTEXTLEN];

//printf("load_filenames 1\n");
		new_edl->create_objects();
//printf("load_filenames 1\n");
		new_edl->copy_session(edl);
//printf("load_filenames 1\n");

		sprintf(string, "Loading %s", new_asset->path);
//printf("load_filenames 1\n");
		gui->show_message(string, BLACK);
//printf("load_filenames 1\n");
		result = new_file->open_file(plugindb, new_asset, 1, 0, 0, 0);
//printf("load_filenames 2\n");

		switch(result)
		{
// Convert media file to EDL
			case FILE_OK:
//printf("load_filenames 1.1\n");
				if(load_mode != LOAD_RESOURCESONLY)
				{
					asset_to_edl(new_edl, new_asset);
//printf("load_filenames 1.2\n");
					new_edls.append(new_edl);
					delete new_asset;
				}
				else
//printf("load_filenames 1.3\n");
				{
					new_assets.append(new_asset);
				}
				if(load_mode == LOAD_REPLACE || 
					load_mode == LOAD_REPLACE_CONCATENATE)
					set_filename("");
				result = 0;
				break;

// File not found
			case FILE_NOT_FOUND:
				sprintf(string, _("Failed to open %s"), new_asset->path);
				gui->show_message(string, RED);
				result = 1;
				break;

// Unknown format
			case FILE_UNRECOGNIZED_CODEC:
			{
// Test index file
				IndexFile indexfile(this);
				result = indexfile.open_index(this, new_asset);
				if(!result)
				{
					indexfile.close_index();
				}

// Test existing EDLs
				if(result)
				{
					for(int j = 0; j < new_edls.total + 1; j++)
					{
						Asset *old_asset;
						if(j == new_edls.total)
						{
							if(old_asset = edl->assets->get_asset(new_asset->path))
							{
								*new_asset = *old_asset;
								result = 0;
							}
						}
						else
						{
							if(old_asset = new_edls.values[j]->assets->get_asset(new_asset->path))
							{
								*new_asset = *old_asset;
								result = 0;
							}
						}
					}
				}

// Prompt user
				if(result)
				{
					char string[BCTEXTLEN];
					FileSystem fs;
					fs.extract_name(string, new_asset->path);

					strcat(string, _("'s format couldn't be determined."));
					new_asset->audio_data = 1;
					new_asset->format = FILE_PCM;
					new_asset->channels = defaults->get("AUDIO_CHANNELS", 2);
					new_asset->sample_rate = defaults->get("SAMPLE_RATE", 44100);
					new_asset->bits = defaults->get("AUDIO_BITS", 16);
					new_asset->byte_order = defaults->get("BYTE_ORDER", 1);
					new_asset->signed_ = defaults->get("SIGNED_", 1);
					new_asset->header = defaults->get("HEADER", 0);

					FileFormat fwindow(this);
					fwindow.create_objects(new_asset, string);
					result = fwindow.run_window();

					defaults->update("AUDIO_CHANNELS", new_asset->channels);
					defaults->update("SAMPLE_RATE", new_asset->sample_rate);
					defaults->update("AUDIO_BITS", new_asset->bits);
					defaults->update("BYTE_ORDER", new_asset->byte_order);
					defaults->update("SIGNED_", new_asset->signed_);
					defaults->update("HEADER", new_asset->header);
					save_defaults();
				}

// Append to list
				if(!result)
				{
// Recalculate length
					delete new_file;
					new_file = new File;
					result = new_file->open_file(plugindb, new_asset, 1, 0, 0, 0);

					if(load_mode != LOAD_RESOURCESONLY)
					{
						asset_to_edl(new_edl, new_asset);
						new_edls.append(new_edl);
						delete new_asset;
					}
					else
					{
						new_assets.append(new_asset);
					}
				}
				else
				{
					result = 1;
				}
				break;
			}

			case FILE_IS_XML:
			{
//printf("load_filenames 2\n");
				FileXML xml_file;
				xml_file.read_from_file(filenames->values[i]);
// Load EDL for pasting
//printf("load_filenames 3\n");
				new_edl->load_xml(plugindb, &xml_file, LOAD_ALL);
//printf("load_filenames 3\n");
				if(load_mode == LOAD_REPLACE || 
					load_mode == LOAD_REPLACE_CONCATENATE)
				{
					strcpy(session->filename, filenames->values[i]);
					strcpy(new_edl->local_session->clip_title, filenames->values[i]);
					set_filename(new_edl->local_session->clip_title);
				}
//new_edl->dump();
//printf("load_filenames 2\n");
				new_edls.append(new_edl);
//printf("load_filenames 4\n");
				result = 0;
				break;
			}
		}
//printf("load_filenames 4\n");

		if(result)
		{
			delete new_edl;
			delete new_asset;
		}
//printf("load_filenames 5\n");

		delete new_file;
//printf("load_filenames 6\n");

	}
//printf("MWindow::load_filenames 5 %d\n", new_edls.total);

//sleep(10);


	if(!result) gui->statusbar->default_message();


//printf("MWindow::load_filenames 7 %d\n", new_edls.total);






// Paste them.
// Don't back up here.
	if(new_edls.total)
	{
// For pasting, clear the active region
		if(load_mode == LOAD_PASTE)
		{
			double start = edl->local_session->get_selectionstart();
			double end = edl->local_session->get_selectionend();
			if(!EQUIV(start, end))
				edl->clear(start, 
					end,
					edl->session->labels_follow_edits,
					edl->session->plugins_follow_edits);
		}

		paste_edls(&new_edls, 
			load_mode,
			0,
			-1,
			edl->session->labels_follow_edits, 
			edl->session->plugins_follow_edits);
	}







//printf("MWindow::load_filenames 8 %d\n", new_edls.total);
//sleep(10);

	if(new_assets.total)
	{
		for(int i = 0; i < new_assets.total; i++)
		{
			mainindexes->add_next_asset(new_assets.values[i]);
			edl->assets->update(new_assets.values[i]);
		}


// Start examining next batch of index files
		mainindexes->start_build();
	}

TRACE("MWindow::load_filenames 100");
	update_project(load_mode);
TRACE("MWindow::load_filenames 110");

//printf("MWindow::load_filenames 9\n");
//sleep(10);

	new_edls.remove_all_objects();
	new_assets.remove_all_objects();
//printf("MWindow::load_filenames 10 %d\n", edl->session->audio_module_fragment);

	if(load_mode == LOAD_REPLACE ||
		load_mode == LOAD_REPLACE_CONCATENATE)
		session->changes_made = 0;

UNTRACE
	return 0;
}

void MWindow::create_objects(int want_gui, 
	int want_new,
	char *config_path)
{
	char string[BCTEXTLEN];
	FileSystem fs;
	edl = 0;


	show_splash();

// For some reason, init_signals must come after show_splash or the signals won't
// get trapped.
	init_signals();


	init_menus();
TRACE("MWindow::create_objects 1");
	init_defaults(defaults, config_path);
TRACE("MWindow::create_objects 2");
	init_preferences();
TRACE("MWindow::create_objects 3");
	init_plugins(preferences, plugindb, splash_window);
	if(splash_window) splash_window->operation->update(_("Initializing GUI"));
TRACE("MWindow::create_objects 4");
	init_theme();
// Default project created here
TRACE("MWindow::create_objects 5");
	init_edl();

TRACE("MWindow::create_objects 6");
	init_awindow();
TRACE("MWindow::create_objects 7");
	init_compositor();
TRACE("MWindow::create_objects 8");
	init_levelwindow();
TRACE("MWindow::create_objects 9");
	init_viewer();
TRACE("MWindow::create_objects 10");
	init_cache();
TRACE("MWindow::create_objects 11");
	init_indexes();
TRACE("MWindow::create_objects 12");
	init_channeldb();
TRACE("MWindow::create_objects 13");

	init_gui();
TRACE("MWindow::create_objects 14");
	init_render();
	init_brender();
	mainprogress = new MainProgress(this, gui);
	undo = new MainUndo(this);

	plugin_guis = new ArrayList<PluginServer*>;

TRACE("MWindow::create_objects 15");
	if(session->show_vwindow) vwindow->gui->show_window();
	if(session->show_cwindow) cwindow->gui->show_window();
	if(session->show_awindow) awindow->gui->show_window();
	if(session->show_lwindow) lwindow->gui->show_window();
TRACE("MWindow::create_objects 16");

// 	vwindow->start();
// 	awindow->start();
// 	cwindow->start();
// 	lwindow->start();
//printf("MWindow::create_objects 1");

	gui->mainmenu->load_defaults(defaults);
TRACE("MWindow::create_objects 17");
	gui->mainmenu->update_toggles();
TRACE("MWindow::create_objects 18");
	gui->patchbay->update();
TRACE("MWindow::create_objects 19");
	gui->canvas->draw();
TRACE("MWindow::create_objects 20");
	gui->cursor->draw();
TRACE("MWindow::create_objects 21");
	gui->raise_window();
TRACE("MWindow::create_objects 22");
	gui->show_window();
TRACE("MWindow::create_objects 23");

	hide_splash();
UNTRACE
}


void MWindow::show_splash()
{
#include "data/heroine_logo11_png.h"
	VFrame *frame = new VFrame(heroine_logo11_png);
	BC_DisplayInfo display_info;
	splash_window = new SplashGUI(frame, 
		display_info.get_root_w() / 2 - frame->get_w() / 2,
		display_info.get_root_h() / 2 - frame->get_h() / 2);
	splash_window->create_objects();
}

void MWindow::hide_splash()
{
	if(splash_window)
		delete splash_window;
	splash_window = 0;
}


void MWindow::start()
{
	vwindow->start();
	awindow->start();
	cwindow->start();
	lwindow->start();
	gui->run_window();
}

void MWindow::show_vwindow()
{
	session->show_vwindow = 1;
	vwindow->gui->lock_window("MWindow::show_vwindow");
	vwindow->gui->show_window();
	vwindow->gui->raise_window();
	vwindow->gui->flush();
	vwindow->gui->unlock_window();
	gui->mainmenu->show_vwindow->set_checked(1);
}

void MWindow::show_awindow()
{
	session->show_awindow = 1;
	awindow->gui->lock_window("MWindow::show_awindow");
	awindow->gui->show_window();
	awindow->gui->raise_window();
	awindow->gui->flush();
	awindow->gui->unlock_window();
	gui->mainmenu->show_awindow->set_checked(1);
}

void MWindow::show_cwindow()
{
	session->show_cwindow = 1;
	cwindow->gui->lock_window("MWindow::show_cwindow");
	cwindow->gui->show_window();
	cwindow->gui->raise_window();
	cwindow->gui->flush();
	cwindow->gui->unlock_window();
	gui->mainmenu->show_cwindow->set_checked(1);
}

void MWindow::show_lwindow()
{
	session->show_lwindow = 1;
	lwindow->gui->lock_window("MWindow::show_lwindow");
	lwindow->gui->show_window();
	lwindow->gui->raise_window();
	lwindow->gui->flush();
	lwindow->gui->unlock_window();
	gui->mainmenu->show_lwindow->set_checked(1);
}

void MWindow::tile_windows()
{
	session->default_window_positions();
	gui->default_positions();
}

void MWindow::toggle_loop_playback()
{
	edl->local_session->loop_playback = !edl->local_session->loop_playback;
	set_loop_boundaries();
	save_backup();

	gui->canvas->draw_overlays();
	gui->canvas->flash();
	sync_parameters(CHANGE_PARAMS);
}

void MWindow::set_titles(int value)
{
	edl->session->show_titles = value;
	trackmovement(edl->local_session->track_start);
}

void MWindow::set_auto_keyframes(int value)
{
	gui->lock_window("MWindow::set_auto_keyframes");
	edl->session->auto_keyframes = value;
	gui->mbuttons->edit_panel->keyframe->update(value);
	gui->flush();
	gui->unlock_window();
	cwindow->gui->lock_window("MWindow::set_auto_keyframes");
	cwindow->gui->edit_panel->keyframe->update(value);
	cwindow->gui->flush();
	cwindow->gui->unlock_window();
}

int MWindow::set_editing_mode(int new_editing_mode)
{
	gui->lock_window("MWindow::set_editing_mode");
	edl->session->editing_mode = new_editing_mode;
	gui->mbuttons->edit_panel->editing_mode = edl->session->editing_mode;
	gui->mbuttons->edit_panel->update();
	gui->canvas->update_cursor();
	gui->unlock_window();
	cwindow->gui->lock_window("MWindow::set_editing_mode");
	cwindow->gui->edit_panel->update();
	cwindow->gui->edit_panel->editing_mode = edl->session->editing_mode;
	cwindow->gui->unlock_window();
	return 0;
}


void MWindow::sync_parameters(int change_type)
{

// Sync engines which are playing back
	if(cwindow->playback_engine->is_playing_back)
	{
		if(change_type == CHANGE_PARAMS)
		{
// TODO: block keyframes until synchronization is done
			cwindow->playback_engine->sync_parameters(edl);
		}
		else
// Stop and restart
		{
			int command = cwindow->playback_engine->command->command;
			cwindow->playback_engine->que->send_command(STOP,
				CHANGE_NONE, 
				0,
				0);
// Waiting for tracking to finish would make the restart position more
// accurate but it can't lock the window to stop tracking for some reason.
// Not waiting for tracking gives a faster response but restart position is
// only as accurate as the last tracking update.
			cwindow->playback_engine->interrupt_playback(0);
			cwindow->playback_engine->que->send_command(command,
					change_type, 
					edl,
					1,
					0);
		}
	}
	else
	{
		cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
							change_type,
							edl,
							1);
	}
}

void MWindow::update_caches()
{
	audio_cache->set_edl(edl);
	video_cache->set_edl(edl);
}

void MWindow::show_plugin(Plugin *plugin)
{
	int done = 0;
//printf("MWindow::show_plugin 1\n");
	plugin_gui_lock->lock();
	for(int i = 0; i < plugin_guis->total; i++)
	{
// Pointer comparison
		if(plugin_guis->values[i]->plugin == plugin)
		{
			plugin_guis->values[i]->raise_window();
			done = 1;
			break;
		}
	}

//printf("MWindow::show_plugin 1\n");
	if(!done)
	{
		if(!plugin->track)
		{
			printf("MWindow::show_plugin track not defined.\n");
		}
		PluginServer *server = scan_plugindb(plugin->title,
			plugin->track->data_type);

//printf("MWindow::show_plugin %p %d\n", server, server->uses_gui);
		if(server && server->uses_gui)
		{
			PluginServer *gui = plugin_guis->append(new PluginServer(*server));
// Needs mwindow to do GUI
			gui->set_mwindow(this);
			gui->open_plugin(0, preferences, edl, plugin, -1);
			gui->show_gui();
			plugin->show = 1;
		}
	}
	plugin_gui_lock->unlock();
//printf("MWindow::show_plugin 2\n");
}

void MWindow::hide_plugin(Plugin *plugin, int lock)
{
	if(lock) plugin_gui_lock->lock();
	plugin->show = 0;
	for(int i = 0; i < plugin_guis->total; i++)
	{
//printf("MWindow::hide_plugin 1 %p %p\n", plugin, plugin_guis->values[i]->plugin);
		if(plugin_guis->values[i]->plugin == plugin)
		{
//printf("MWindow::hide_plugin 2\n");
			PluginServer *ptr = plugin_guis->values[i];
			plugin_guis->remove(ptr);
			if(lock) plugin_gui_lock->unlock();
// Last command executed in client side close
			delete ptr;
			return;
		}
	}
	if(lock) plugin_gui_lock->unlock();
}

void MWindow::hide_plugins()
{
	plugin_gui_lock->lock();
	plugin_guis->remove_all_objects();
	plugin_gui_lock->unlock();
}

void MWindow::update_plugin_guis()
{
	plugin_gui_lock->lock();

	for(int i = 0; i < plugin_guis->total; i++)
	{
		plugin_guis->values[i]->update_gui();
	}
	plugin_gui_lock->unlock();
}

void MWindow::render_plugin_gui(void *data, Plugin *plugin)
{
//printf("MWindow::render_plugin_gui 1\n");
	plugin_gui_lock->lock();
	for(int i = 0; i < plugin_guis->total; i++)
	{
		if(plugin_guis->values[i]->plugin->identical_location(plugin))
		{
//printf("MWindow::render_plugin_gui 2\n");
			plugin_guis->values[i]->render_gui(data);
			break;
		}
	}
	plugin_gui_lock->unlock();
}

void MWindow::render_plugin_gui(void *data, int size, Plugin *plugin)
{
//printf("MWindow::render_plugin_gui 1\n");
	plugin_gui_lock->lock();
	for(int i = 0; i < plugin_guis->total; i++)
	{
		if(plugin_guis->values[i]->plugin->identical_location(plugin))
		{
//printf("MWindow::render_plugin_gui 2\n");
			plugin_guis->values[i]->render_gui(data, size);
			break;
		}
	}
	plugin_gui_lock->unlock();
}


void MWindow::update_plugin_states()
{
	int result = 0;
	plugin_gui_lock->lock();
	for(int i = 0; i < plugin_guis->total; i++)
	{
// Get a plugin GUI
		Plugin *src_plugin = plugin_guis->values[i]->plugin;
		PluginServer *src_plugingui = plugin_guis->values[i];

// Search for plugin in EDL.  Only the master EDL shows plugin GUIs.
		for(Track *track = edl->tracks->first; 
			track && !result; 
			track = track->next)
		{
			for(int j = 0; 
				j < track->plugin_set.total && !result; 
				j++)
			{
				PluginSet *plugin_set = track->plugin_set.values[j];
				for(Plugin *plugin = (Plugin*)plugin_set->first; 
					plugin && !result; 
					plugin = (Plugin*)plugin->next)
				{
					if(plugin == src_plugin &&
						!strcmp(plugin->title, src_plugingui->title)) result = 1;
				}
			}
		}


// Doesn't exist anymore
		if(!result)
		{
			hide_plugin(src_plugin, 0);
			i--;
		}
	}
	plugin_gui_lock->unlock();
}


void MWindow::update_plugin_titles()
{
	for(int i = 0; i < plugin_guis->total; i++)
	{
		plugin_guis->values[i]->update_title();
	}
}

int MWindow::asset_to_edl(EDL *new_edl, 
	Asset *new_asset, 
	RecordLabels *labels)
{
//printf("MWindow::asset_to_edl 1\n");
//	new_edl->load_defaults(defaults);

// Keep frame rate, sample rate, and output size unchanged.
// These parameters would revert the project if VWindow displayed an asset
// of different size than the project.
	if(new_asset->video_data)
	{
		new_edl->session->video_tracks = new_asset->layers;
//		new_edl->session->frame_rate = new_asset->frame_rate;
//		new_edl->session->output_w = new_asset->width;
//		new_edl->session->output_h = new_asset->height;
	}
	else
		new_edl->session->video_tracks = 0;






	if(new_asset->audio_data)
	{
		new_edl->session->audio_tracks = new_asset->channels;
//		new_edl->session->sample_rate = new_asset->sample_rate;
	}
	else
		new_edl->session->audio_tracks = 0;
//printf("MWindow::asset_to_edl 2 %d %d\n", new_edl->session->video_tracks, new_edl->session->audio_tracks);

	new_edl->create_default_tracks();
//printf("MWindow::asset_to_edl 2 %d %d\n", new_edl->session->video_tracks, new_edl->session->audio_tracks);

// Disable drawing if the file format isn't fast enough.
	if(new_asset->format == FILE_MPEG)
	{
		for(Track *current = new_edl->tracks->first;
			current;
			current = NEXT)
		{
			if(current->data_type == TRACK_VIDEO) current->draw = 0;
		}
	}



//printf("MWindow::asset_to_edl 3\n");
	new_edl->insert_asset(new_asset, 0, 0, labels);
//printf("MWindow::asset_to_edl 3\n");





	char string[BCTEXTLEN];
	FileSystem fs;
	fs.extract_name(string, new_asset->path);
//printf("MWindow::asset_to_edl 3\n");

	strcpy(new_edl->local_session->clip_title, string);
//printf("MWindow::asset_to_edl 4 %s\n", string);

//	new_edl->dump();
	return 0;
}

// Reset everything after a load.
void MWindow::update_project(int load_mode)
{
	restart_brender();
//TRACE("MWindow::update_project 1");
	edl->tracks->update_y_pixels(theme);

// Draw timeline
//TRACE("MWindow::update_project 2");
	update_caches();

TRACE("MWindow::update_project 3");
	gui->update(1, 1, 1, 1, 1, 1, 1);

TRACE("MWindow::update_project 4");
	cwindow->update(0, 0, 1, 1, 1);

TRACE("MWindow::update_project 5");

	if(load_mode == LOAD_REPLACE ||
		load_mode == LOAD_REPLACE_CONCATENATE)
	{
		vwindow->change_source();
	}
	else
	{
		vwindow->update(1);
	}

TRACE("MWindow::update_project 6");

	cwindow->gui->slider->set_position();
TRACE("MWindow::update_project 6.1");
	cwindow->gui->timebar->update(1, 1);
TRACE("MWindow::update_project 6.2");
	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
		CHANGE_ALL,
		edl,
		1);

TRACE("MWindow::update_project 7");
	awindow->gui->lock_window("MWindow::update_project");
	awindow->gui->update_assets();
	awindow->gui->flush();
	awindow->gui->unlock_window();
	gui->flush();
TRACE("MWindow::update_project 100");
}


void MWindow::rebuild_indices()
{
	char source_filename[BCTEXTLEN], index_filename[BCTEXTLEN];

	for(int i = 0; i < session->drag_assets->total; i++)
	{
//printf("MWindow::rebuild_indices 1 %s\n", session->drag_assets->values[i]->path);
// Erase file
		IndexFile::get_index_filename(source_filename, 
			preferences->index_directory,
			index_filename, 
			session->drag_assets->values[i]->path);
		remove(index_filename);
// Schedule index build
		session->drag_assets->values[i]->index_status = INDEX_NOTTESTED;
		mainindexes->add_next_asset(session->drag_assets->values[i]);
	}
	mainindexes->start_build();
}


void MWindow::save_backup()
{
	FileXML file;
	edl->save_xml(plugindb, 
		&file, 
		BACKUP_PATH,
		0,
		0);
	file.terminate_string();
	char path[BCTEXTLEN];
	FileSystem fs;
	strcpy(path, BACKUP_PATH);
	fs.complete_path(path);

	if(file.write_to_file(path))
	{
		char string2[256];
		sprintf(string2, _("Couldn't open %s for writing."), BACKUP_PATH);
		gui->show_message(string2);
	}
}


int MWindow::create_aspect_ratio(float &w, float &h, int width, int height)
{
	int denominator;
	float fraction = (float)width / height;

	for(denominator = 1; 
		denominator < 100 && 
			fabs(fraction * denominator - (int)(fraction * denominator)) > .001; 
		denominator++)
		;

	w = denominator * width / height;
	h = denominator;
}



void MWindow::remove_assets_from_project(int push_undo)
{
    if(push_undo) undo->update_undo_before(_("remove assets"), LOAD_ALL);

// Remove from caches
	for(int i = 0; i < session->drag_assets->total; i++)
	{
		audio_cache->delete_entry(session->drag_assets->values[i]);
		video_cache->delete_entry(session->drag_assets->values[i]);
	}

printf("MWindow::remove_assets_from_project 1\n");
video_cache->dump();
audio_cache->dump();
printf("MWindow::remove_assets_from_project 100\n");

// Remove from VWindow.
	for(int i = 0; i < session->drag_clips->total; i++)
	{
		if(session->drag_clips->values[i] == vwindow->get_edl())
		{
			vwindow->gui->lock_window("MWindow::remove_assets_from_project 1");
			vwindow->remove_source();
			vwindow->gui->unlock_window();
		}
	}
	
	for(int i = 0; i < session->drag_assets->total; i++)
	{
		if(session->drag_assets->values[i] == vwindow->get_asset())
		{
			vwindow->gui->lock_window("MWindow::remove_assets_from_project 2");
			vwindow->remove_source();
			vwindow->gui->unlock_window();
		}
	}
	
	edl->remove_from_project(session->drag_assets);
	edl->remove_from_project(session->drag_clips);
	save_backup();
	if(push_undo) undo->update_undo_after();
	restart_brender();

	gui->lock_window("MWindow::remove_assets_from_project 3");
	gui->update(1,
		1,
		1,
		1,
		0, 
		1,
		0);
	gui->unlock_window();

	awindow->gui->lock_window("MWindow::remove_assets_from_project 4");
	awindow->gui->update_assets();
	awindow->gui->flush();
	awindow->gui->unlock_window();

// Removes from playback here
	sync_parameters(CHANGE_ALL);
}

void MWindow::remove_assets_from_disk()
{
// Remove from disk
	for(int i = 0; i < session->drag_assets->total; i++)
	{
		remove(session->drag_assets->values[i]->path);
	}

	remove_assets_from_project(1);
}

void MWindow::dump_plugins()
{
	for(int i = 0; i < plugindb->total; i++)
	{
		printf("audio=%d video=%d realtime=%d transition=%d theme=%d %s\n",
			plugindb->values[i]->audio,
			plugindb->values[i]->video,
			plugindb->values[i]->realtime,
			plugindb->values[i]->transition,
			plugindb->values[i]->theme,
			plugindb->values[i]->title);
	}
}

























int MWindow::save_defaults()
{
	gui->save_defaults(defaults);
	edl->save_defaults(defaults);
	session->save_defaults(defaults);
	preferences->save_defaults(defaults);

	defaults->save();
	return 0;
}

int MWindow::run_script(FileXML *script)
{
	int result = 0, result2 = 0;
	while(!result && !result2)
	{
		result = script->read_tag();
		if(!result)
		{
			if(script->tag.title_is("new_project"))
			{
// Run new in immediate mode.
//				gui->mainmenu->new_project->run_script(script);
			}
			else
			if(script->tag.title_is("record"))
			{
// Run record as a thread.  It is a terminal command.
				;
// Will read the complete scipt file without letting record read it if not
// terminated.
				result2 = 1;
			}
			else
			{
				printf("MWindow::run_script: Unrecognized command: %s\n",script->tag.get_title() );
			}
		}
	}
	return result2;
}

// ================================= synchronization


int MWindow::interrupt_indexes()
{
	mainindexes->interrupt_build();
	return 0; 
}



void MWindow::next_time_format()
{
	switch(edl->session->time_format)
	{
		case 0: edl->session->time_format = 1; break;
		case 1: edl->session->time_format = 2; break;
		case 2: edl->session->time_format = 3; break;
		case 3: edl->session->time_format = 4; break;
		case 4: edl->session->time_format = 5; break;
		case 5: edl->session->time_format = 0; break;
	}

	gui->lock_window();
	gui->redraw_time_dependancies();



	char string[BCTEXTLEN], string2[BCTEXTLEN];
	sprintf(string, _("Using %s."), Units::print_time_format(edl->session->time_format, string2));
	gui->show_message(string, BLACK);
	gui->flush();
	gui->unlock_window();
}

int MWindow::set_filename(char *filename)
{
	strcpy(session->filename, filename);
	if(gui)
	{
		if(filename[0] == 0)
		{
			gui->set_title(PROGRAM_NAME);
		}
		else
		{
			FileSystem dir;
			char string[BCTEXTLEN], string2[BCTEXTLEN];
			dir.extract_name(string, filename);
			sprintf(string2, PROGRAM_NAME ": %s", string);
			gui->set_title(string2);
		}
	}
	return 0; 
}








int MWindow::set_loop_boundaries()
{
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();
	
	if(start != 
		end) 
	{
		;
	}
	else
	if(edl->tracks->total_length())
	{
		start = 0;
		end = edl->tracks->total_length();
	}
	else
	{
		start = end = 0;
	}

	if(edl->local_session->loop_playback && start != end)
	{
		edl->local_session->loop_start = start;
		edl->local_session->loop_end = end;
	}
	return 0; 
}







int MWindow::reset_meters()
{
	cwindow->gui->lock_window("MWindow::reset_meters 1");
	cwindow->gui->meters->reset_meters();
	cwindow->gui->unlock_window();

	vwindow->gui->lock_window("MWindow::reset_meters 2");
	vwindow->gui->meters->reset_meters();
	vwindow->gui->unlock_window();

	lwindow->gui->lock_window("MWindow::reset_meters 3");
	lwindow->gui->panel->reset_meters();
	lwindow->gui->unlock_window();

	gui->lock_window("MWindow::reset_meters 4");
	gui->patchbay->reset_meters();
	gui->unlock_window();
	return 0; 
}


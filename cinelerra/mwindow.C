
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
#include "assetlist.h"
#include "awindowgui.h"
#include "awindow.h"
#include "batchrender.h"
#include "bcdisplayinfo.h"
#include "bcprogress.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "brender.h"
#include "cache.h"
#include "cinelerra.h"
#include "clip.h"
#include "cliplist.h"
#include "colormodels.h"
#include "cplayback.h"
#include "ctimebar.h"
#include "cwindowgui.h"
#include "cwindow.h"
#include "bchash.h"
#include "editpanel.h"
#include "edl.h"
#include "edlsession.h"
#include "fileformat.h"
#include "file.h"
#include "filesystem.h"
#include "filexml.h"
#include "framecache.h"
#include "glthread.h"
#include "gwindow.h"
#include "gwindowgui.h"
#include "indexfile.h"
#include "language.h"
#include "levelwindowgui.h"
#include "levelwindow.h"
#include "loadfile.inc"
#include "loadmode.h"
#include "localsession.h"
#include "maincursor.h"
#include "mainerror.h"
#include "mainindexes.h"
#include "mainmenu.h"
#include "mainprogress.h"
#include "mainsession.h"
#include "mainundo.h"
#include "maplist.h"
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
#include "render.h"
#include "ruler.h"
#include "samplescroll.h"
#include "sighandler.h"
#include "splashgui.h"
#include "statusbar.h"
#include "theme.h"
#include "timebar.h"
#include "tipwindow.h"
#include "tmpframecache.h"
#include "trackcanvas.h"
#include "track.h"
#include "tracking.h"
#include "tracks.h"
#include "transition.h"
#include "transportcommand.h"
#include "vdeviceprefs.h"
#include "vframe.h"
#include "videodevice.inc"
#include "vplayback.h"
#include "vwindowgui.h"
#include "vwindow.h"
#include "wavecache.h"
#include "zoombar.h"
#include "exportedl.h"
#include "ntsczones.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

extern const char *version_name;

MWindow::MWindow(const char *config_path)
 : Thread(THREAD_SYNCHRONOUS)
{
	char string[BCTEXTLEN];

	plugin_gui_lock = new Mutex("MWindow::plugin_gui_lock");
	brender_lock = new Mutex("MWindow::brender_lock");
	brender = 0;
	init_signals();
	mwindow_global = this;

	glthread = new GLThread();


	show_splash();

	init_defaults(defaults, config_path);
	default_standard = default_std();

	init_edl();
	init_preferences();
	init_plugins(preferences, plugindb, splash_window);
	if(splash_window) splash_window->operation->update(_("Initializing GUI"));
	init_theme();
	init_error();

	strcpy(string, preferences->global_plugin_dir);
	strcat(string, "/" FONT_SEARCHPATH);
	BC_Resources::init_fontconfig(string);
	last_backup_time = time(0);

	init_awindow();
	init_compositor();
	init_levelwindow();
	init_viewer();
	init_ruler();
	init_cache();
	init_indexes();

	init_gui();
	init_gwindow();

	init_render();
	init_brender();
	init_exportedl();
	mainprogress = new MainProgress(this, gui);
	undo = new MainUndo(this);

	plugin_guis = new ArrayList<PluginServer*>;
	removed_guis = new ArrayList<PluginServer*>;

	if(mainsession->show_vwindow) vwindow->gui->show_window();
	if(mainsession->show_cwindow) cwindow->gui->show_window();
	if(mainsession->show_awindow) awindow->gui->show_window();
	if(mainsession->show_lwindow) lwindow->gui->show_window();
	if(mainsession->show_gwindow) gwindow->gui->show_window();
	if(mainsession->show_ruler) ruler->gui->show_window();

	gui->mainmenu->load_defaults(defaults);
	gui->mainmenu->update_toggles();
	gui->patchbay->update();
	gui->canvas->draw();
	gui->cursor->draw(1);
	gui->show_window();
	gui->raise_window();

	if(preferences->use_tipwindow)
		init_tipwindow();
	hide_splash();
}

MWindow::~MWindow()
{
	delete_brender();
	delete brender_lock;

	delete mainindexes;

	clean_indexes();

	save_defaults();
// Give up and go to a movie
	exit(0);

	delete mainprogress;
	delete audio_cache;             // delete the cache after the assets
	delete video_cache;             // delete the cache after the assets
	delete frame_cache;
	if(gui) delete gui;
	delete undo;
	delete preferences;
	delete defaults;
	delete render;
	delete awindow;
	delete vwindow;
	delete cwindow;
	delete lwindow;
	plugin_guis->remove_all_objects();
	removed_guis->remove_all_objects();
	delete plugin_guis;
	delete removed_guis;
	delete plugin_gui_lock;
	delete vwindow_edl;
	delete master_edl;
}

void MWindow::init_error()
{
	new MainError(this);
}

void MWindow::create_defaults_path(char *string)
{
// set the .bcast path
	FileSystem fs;

	strcpy(string, BCASTDIR);
	fs.complete_path(string);
	if(!fs.is_dir(string)) 
	{
		fs.create_dir(string); 
	}

// load the defaults
	strcat(string, CONFIG_FILE);
}

const char *MWindow::default_std()
{
	int fd, i, l = 0;
	char buf[BCTEXTLEN];
	char *p;

	if((fd = open(TIMEZONE_NAME, O_RDONLY)) >= 0)
	{
		l = read(fd, buf, BCTEXTLEN);
		close(fd);
		if(l > 0){
			buf[l] = 0;
			if(buf[l - 1] == '\n')
				buf[--l] = 0;
		}
		p = buf;
	}
	if(l < 1)
	{
		if((l = readlink(LOCALTIME_LINK, buf, BCTEXTLEN)) > 0)
		{
			buf[l] = 0;
			if(p = strstr(buf, ZONEINFO_STR))
			{
				p += strlen(ZONEINFO_STR);
				l = strlen(p);
			}
			else
				l = 0;
		}
	}
	if(l)
	{
		for(i = 0; ntsc_zones[i]; i++)
			if(strcmp(ntsc_zones[i], p) == 0)
				return "NTSC";
	}
	return "PAL";
}

char *MWindow::create_title(const char *name)
{
	static char title_string[BCTEXTLEN];

	sprintf(title_string, "%s - " PROGRAM_NAME, _(name));
	return title_string;
}

void MWindow::init_defaults(BC_Hash* &defaults, const char *config_path)
{
	char path[BCTEXTLEN];
// Use user supplied path
	if(config_path[0])
	{
		strcpy(path, config_path);
	}
	else
	{
		create_defaults_path(path);
	}
	defaults = new BC_Hash(path);
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
			if(fs->is_dir(path))
			{
				continue;
			}
			else
			{
// Try to query the plugin
				fs->complete_path(path);
				PluginServer *new_plugin = new PluginServer(path);
				int result = new_plugin->open_plugin(1, preferences, 0, 0);

				if(!result)
				{
					plugindb->append(new_plugin);
					new_plugin->close_plugin();
					new_plugin->release_plugin();
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
							new_plugin->release_plugin();
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
		errorbox(_("Couldn't open plugins directory '%s'"),
			preferences->global_plugin_dir);
	}

#ifdef HAVE_LADSPA
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
					errorbox(_("Couldn't open LADSPA directory '%s'"),
						string);
				}
			}

			if(ptr)
				ptr1 = ptr + 1;
			else
				ptr1 = ptr;
		};
	}
#endif

	int total = cinelerra_fs.total_files();
	int counter = 0;
	for(int i = 0; i < lad_fs.total; i++)
		total += lad_fs.values[i]->total_files();
	if(splash_window) splash_window->progress->update_length(total);

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
		int is_multichannel,
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
			(current->multichannel == is_multichannel || is_multichannel < 0) &&
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
			if(strcmp(_(value1->title), _(value2->title)) > 0)
			{
				done = 0;
				plugindb.values[i] = value2;
				plugindb.values[i + 1] = value1;
			}
		}
	}
}

PluginServer* MWindow::scan_plugindb(const char *title,
		int data_type)
{
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

void MWindow::dump_plugindb(int data_type)
{
	int typ;

	for(int i = 0; i < plugindb->total; i++)
	{
		PluginServer *server = plugindb->values[i];
		if(data_type > 0)
		{
			if(data_type == TRACK_AUDIO && !server->audio)
				continue;
			if(data_type == TRACK_VIDEO && !server->video)
				continue;
		}
		if(server->audio)
			typ = 'A';
		else if(server->video)
			typ = 'V';
		else if(server->theme)
			typ = 'T';
		else
			typ = '-';
		printf("    %c%c%c%c%c%c%d %s\n",
			server->realtime ? 'R' : '-', typ,
			server->uses_gui ? 'G' : '-', server->multichannel ? 'M' : '-', 
			server->synthesis ? 'S' : '-', server->transition ? 'T' : '-', 
			server->apiversion, server->title);
	}
}

void MWindow::init_preferences()
{
	preferences = new Preferences;
	preferences->load_defaults(defaults);
	mainsession = new MainSession();
	mainsession->load_defaults(defaults);
	preferences_global = preferences;
}

void MWindow::clean_indexes()
{
	FileSystem fs;
	int total_excess;
	long oldest;
	int oldest_item = -1;
	int result;
	char string[BCTEXTLEN];
	char string2[BCTEXTLEN];

// Delete extra indexes
	fs.set_filter("*" INDEXFILE_EXTENSION);
	fs.complete_path(preferences->index_directory);
	fs.update(preferences->index_directory);

// Eliminate directories
	result = 1;
	while(result)
	{
		result = 0;
		for(int i = 0; i < fs.dir_list.total && !result; i++)
		{
			fs.join_names(string, preferences->index_directory, fs.dir_list.values[i]->name);
			if(fs.is_dir(string))
			{
				delete fs.dir_list.values[i];
				fs.dir_list.remove_number(i);
				result = 1;
			}
		}
	}
	total_excess = fs.dir_list.total - preferences->index_count;

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

		if(oldest_item >= 0)
		{
// Remove index file
			fs.join_names(string, 
				preferences->index_directory, 
				fs.dir_list.values[oldest_item]->name);
			if(remove(string))
				perror("delete_indexes");
			delete fs.dir_list.values[oldest_item];
			fs.dir_list.remove_number(oldest_item);

// Remove table of contents if it exists
			strcpy(string2, string);
			char *ptr = strrchr(string2, '.');
			if(ptr)
			{
				strcpy(ptr, TOCFILE_EXTENSION);
				remove(string2);
			}
		}

		total_excess--;
	}
}

void MWindow::init_awindow()
{
	awindow = new AWindow(this);
}

void MWindow::init_gwindow()
{
	gwindow = new GWindow(this);
}

void MWindow::init_tipwindow()
{
	twindow = new TipWindow(this);
	twindow->start();
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
			plugin.open_plugin(0, preferences, 0, 0);
			theme = plugin.new_theme();
			theme->mwindow = this;
			plugin.close_plugin();
		}
	}

	if(!theme)
	{
		errorbox(_("Can't find theme '%s'."), preferences->theme);
		// Theme load fails, try default
		strcpy(preferences->theme, DEFAULT_THEME);
		for(int i = 0; i < plugindb->total; i++)
		{
			if(plugindb->values[i]->theme &&
				!strcasecmp(preferences->theme, plugindb->values[i]->title))
			{
				PluginServer plugin = *plugindb->values[i];
				plugin.open_plugin(0, preferences, 0, 0);
				theme = plugin.new_theme();
				theme->mwindow = this;
				plugin.close_plugin();
			}
		}
	}

	if(!theme)
	{
		errorbox(_("Cant load default theme '%s'."), preferences->theme);
		exit(1);
	}

// Load images which may have been forgotten
	theme->Theme::initialize();
// Load user images
	theme->initialize();

	theme->check_used();
	theme_global = theme;
	master_edl->tracks->update_y_pixels(theme);
}

void MWindow::init_edl()
{
	master_edl = new EDL(1);
	edlsession = new EDLSession();
	vwindow_edl = new EDL(0);

	FormatPresets::fill_preset_defaults(default_standard, edlsession);
	master_edl->load_defaults(defaults, edlsession);
	master_edl->create_default_tracks();
}

void MWindow::init_compositor()
{
	cwindow = new CWindow(this);
}

void MWindow::init_levelwindow()
{
	lwindow = new LevelWindow(this);
}

void MWindow::init_viewer()
{
	vwindow = new VWindow(this);
}

void MWindow::init_cache()
{
	audio_cache = new CICache(preferences, FILE_OPEN_AUDIO);
	video_cache = new CICache(preferences, FILE_OPEN_VIDEO);
	frame_cache = new FrameCache;
	wave_cache = new WaveCache;
}

void MWindow::init_ruler()
{
	ruler = new Ruler(this);
}

void MWindow::init_indexes()
{
	mainindexes = new MainIndexes(this);
	mainindexes->start_loop();
}

void MWindow::init_gui()
{
	gui = new MWindowGUI(this);
	gui->show();
	gui->load_defaults(defaults);
}

void MWindow::init_signals()
{
	sighandler = new SigHandler;
	sighandler->initialize();
	sighandler->initXErrors();
}

void MWindow::init_render()
{
	render = new Render(this);
	batch_render = new BatchRenderThread(this);
}

void MWindow::init_exportedl()
{
	exportedl = new ExportEDL(this);
}

void MWindow::init_brender()
{
	if(preferences->use_brender && !brender)
	{
		brender_lock->lock("MWindow::init_brender");
		brender = new BRender(this);
		brender->initialize();
		mainsession->brender_end = 0;
		brender_lock->unlock();
		brender->restart(master_edl);
	}
	else
	if(!preferences->use_brender && brender)
		delete_brender();
}

void MWindow::delete_brender()
{
	brender_lock->lock("MWindow::delete_brender");
	if(brender)
	{
		delete brender;
		brender = 0;
		mainsession->brender_end = 0;
	}
	brender_lock->unlock();
}

void MWindow::restart_brender()
{
	if(brender) brender->restart(master_edl);
}

void MWindow::stop_brender()
{
	if(brender) brender->stop();
}

int MWindow::brender_available(ptstime postime)
{
	int result = 0;

	brender_lock->lock("MWindow::brender_available 1");
	if(brender)
	{
		if(brender->map_valid)
		{
			brender->map_lock->lock("MWindow::brender_available 2");
			if(postime >= 0)
				result = brender->videomap.is_set(postime);
			brender->map_lock->unlock();
		}
	}
	brender_lock->unlock();
	return result;
}

void MWindow::set_brender_start()
{
	edlsession->brender_start = master_edl->local_session->get_selectionstart();
	restart_brender();
	gui->canvas->draw_overlays();
	gui->canvas->flash();
}

void MWindow::load_filenames(ArrayList<char*> *filenames, 
	int load_mode,
	int update_filename)
{
	int result = 0;
	ptstime pos, dur;
	int oloadmode = load_mode;

	save_defaults();
	gui->start_hourglass();

// Stop playback EDL-s are going modified
	cwindow->stop_playback();
	vwindow->stop_playback();

	if(load_mode == LOADMODE_REPLACE || load_mode == LOADMODE_REPLACE_CONCATENATE)
	{
		reset_caches();
		hide_plugins();
		assetlist_global.reset_inuse();
		vwindow->remove_source();
		vwindow_edl->reset_instance();
		master_edl->reset_instance();
	}

	for(int i = 0; i < filenames->total; i++)
	{
// Get type of file
		File *new_file = new File;
		Asset *new_asset;
		EDL *new_edl = 0;
		Asset *next_asset;

		if(new_asset = assetlist_global.get_asset(filenames->values[i]))
		{
			if(load_mode != LOADMODE_REPLACE &&
				load_mode != LOADMODE_REPLACE_CONCATENATE)
			continue;
		}
		else
		{
			new_asset = new Asset(filenames->values[i]);
			gui->show_message("Loading %s", new_asset->path);
		}
// Open all streams
		result = new_file->open_file(new_asset, FILE_OPEN_READ | FILE_OPEN_ALL);

		switch(result)
		{
// Convert media file to EDL
		case FILE_OK:
// Warn about odd image dimensions
			if(!new_asset->single_image && new_asset->video_data &&
				((new_asset->width % 2) ||
				(new_asset->height % 2)))
			{
				errormsg("%s's\nresolution is %dx%d. Images with odd dimensions may not decode properly.",
					new_asset->path,
					new_asset->width,
					new_asset->height);
			}

			if(new_asset->nb_programs)
			{
				delete new_file;
				new_file = 0;

				for(i = 0; i < new_asset->nb_programs; i++)
				{
					new_asset->set_program(i);
					next_asset = new Asset();
					next_asset->copy_from(new_asset, 0);
					new_asset = assetlist_global.add_asset(new_asset);
					mainindexes->add_next_asset(new_asset);

					if(load_mode != LOADMODE_RESOURCESONLY)
					{
						master_edl->update_assets(new_asset);
						switch(load_mode)
						{
						case LOADMODE_REPLACE:
							master_edl->finalize_edl(load_mode);
							load_mode = LOADMODE_NEW_TRACKS;
							break;
						case LOADMODE_REPLACE_CONCATENATE:
							master_edl->finalize_edl(load_mode);
							load_mode = LOADMODE_CONCATENATE;
							break;
						case LOADMODE_NEW_TRACKS:
							master_edl->tracks->create_new_tracks(new_asset);
							break;
						case LOADMODE_CONCATENATE:
							master_edl->tracks->append_asset(new_asset);
							break;
						case LOADMODE_PASTE:
							pos = master_edl->local_session->get_selectionstart();
							dur = master_edl->tracks->append_asset(new_asset,
								master_edl->local_session->get_selectionstart());
							master_edl->local_session->set_selection(pos + dur);
							break;
						}
					}
					new_asset = next_asset;
				}
				new_asset = 0;
			}
			else
			{
				delete new_file;
				new_file = 0;
				new_asset = assetlist_global.add_asset(new_asset);
				mainindexes->add_next_asset(new_asset);

				if(load_mode != LOADMODE_RESOURCESONLY)
				{
					master_edl->update_assets(new_asset);

					switch(load_mode)
					{
					case LOADMODE_REPLACE:
						master_edl->finalize_edl(load_mode);
						load_mode = LOADMODE_NEW_TRACKS;
						break;
					case LOADMODE_CONCATENATE:
						master_edl->tracks->append_asset(new_asset);
						break;
					case LOADMODE_NEW_TRACKS:
						master_edl->tracks->create_new_tracks(new_asset);
						break;
					case LOADMODE_PASTE:
						pos = master_edl->local_session->get_selectionstart();
						dur = master_edl->tracks->append_asset(new_asset,
							master_edl->local_session->get_selectionstart());
						master_edl->local_session->set_selection(pos + dur);
						break;
					case LOADMODE_REPLACE_CONCATENATE:
						master_edl->finalize_edl(load_mode);
						load_mode = LOADMODE_CONCATENATE;
						break;
					}
				}
				new_asset = 0;
			}

			result = 0;
			break;

// File not found
		case FILE_NOT_FOUND:
			errormsg(_("Failed to open %s"), new_asset->path);
			result = 1;
			delete new_file;
			delete new_asset;
			new_file = 0;
			new_asset = 0;
			break;

// Unknown format
		case FILE_UNRECOGNIZED_CODEC:
		{
// Test index file
			IndexFile indexfile(this);
			new_asset->nb_streams = 0;
			new_asset->audio_streamno = 1;
			result = indexfile.open_index(new_asset);
			if(!result)
			{
				indexfile.close_index();
				new_asset->init_streams();
			}
// Test known assets
			if(result)
			{
				Asset *old_asset = assetlist_global.get_asset(new_asset->path);
				if(old_asset)
				{
					delete new_asset;
					new_asset = old_asset;
					result = 0;
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
				new_asset->nb_streams = 0;

				FileFormat fwindow(new_asset, string);
				result = fwindow.run_window();
				if(!result)
				{
					if(SampleRateSelection::limits(&new_asset->sample_rate) < 0)
						errorbox(_("Sample rate is out of limits (%d..%d).\nCorrection applied."),
							MIN_SAMPLE_RATE, MAX_SAMPLE_RATE);

					defaults->update("AUDIO_CHANNELS", new_asset->channels);
					defaults->update("SAMPLE_RATE", new_asset->sample_rate);
					defaults->update("AUDIO_BITS", new_asset->bits);
					defaults->update("BYTE_ORDER", new_asset->byte_order);
					defaults->update("SIGNED_", new_asset->signed_);
					defaults->update("HEADER", new_asset->header);
					save_defaults();
					new_asset->init_streams();
				}
			}

// Append to list
			if(!result)
			{
// Recalculate length
				delete new_file;
				new_file = new File;
				result = new_file->open_file(new_asset, FILE_OPEN_READ | FILE_OPEN_ALL);

				if(!result)
				{
					new_asset = assetlist_global.add_asset(new_asset);
					mainindexes->add_next_asset(new_asset);
					if(load_mode != LOADMODE_RESOURCESONLY)
					{
						master_edl->update_assets(new_asset);
						switch(load_mode)
						{
						case LOADMODE_CONCATENATE:
							master_edl->tracks->append_asset(new_asset);
							break;
						case LOADMODE_NEW_TRACKS:
							master_edl->tracks->create_new_tracks(new_asset);
							break;
						case LOADMODE_PASTE:
							master_edl->tracks->append_asset(new_asset,
								master_edl->local_session->get_selectionstart());
							break;
						case LOADMODE_REPLACE_CONCATENATE:
							master_edl->finalize_edl(load_mode);
							load_mode = LOADMODE_CONCATENATE;
							break;
						case LOADMODE_REPLACE:
							master_edl->finalize_edl(load_mode);
							load_mode = LOADMODE_NEW_TRACKS;
							break;
						}
					}
					new_asset = 0;
				}
				delete new_file;
				new_file = 0;
			}
			else
			{
				result = 1;
			}
			break;
		}

		case FILE_IS_XML:
			{
				EDL *cur_edl;
				FileXML xml_file;
				xml_file.read_from_file(filenames->values[i]);
				result = 0;

				if(load_mode == LOADMODE_REPLACE ||
					load_mode == LOADMODE_REPLACE_CONCATENATE)
				{
					reset_caches();
					hide_plugins();
					master_edl->reset_instance();
					master_edl->load_xml(&xml_file, LOAD_ALL, edlsession);
					strcpy(mainsession->filename, filenames->values[i]);
					strcpy(master_edl->local_session->clip_title, filenames->values[i]);
					if(update_filename)
						set_filename(master_edl->local_session->clip_title);
					cur_edl = master_edl;
					new_edl = 0;
					if(load_mode == LOADMODE_REPLACE_CONCATENATE)
						load_mode = LOADMODE_CONCATENATE;
					else
						load_mode = LOADMODE_NEW_TRACKS;
					master_edl->finalize_edl(load_mode);
					gui->mainmenu->update_toggles();
					gwindow->gui->update_toggles();
				}
				else
				{
// Load EDL for pasting
					new_edl = new EDL(0);
					new_edl->copy_session(master_edl);
					new_edl->load_xml(&xml_file, LOAD_ALL, 0);
					cur_edl = new_edl;
				}
				test_plugins(cur_edl, filenames->values[i]);

// Open media files found in xml - open fills media info
				for(int i = 0; i < cur_edl->assets->total; i++)
				{
					Asset *current = cur_edl->assets->values[i];
					delete new_file;
					new_file = new File;
					if(new_file->open_file(current, FILE_OPEN_READ | FILE_OPEN_ALL) != FILE_OK)
					{
						result++;
						break;
					}
					else
						mainindexes->add_next_asset(current);
				}
				delete new_file;
				new_file = 0;
				if(!result && new_edl)
				{
					switch(load_mode)
					{
					case LOADMODE_RESOURCESONLY:
						cliplist_global.add_clip(new_edl);
						new_edl = 0;
						break;
					default:
						paste_edl(new_edl, load_mode,
							0, -1,
							edlsession->edit_actions(), 0);
					}
					delete new_edl;
					new_edl = 0;
				}
				else if(result && update_filename)
					set_filename("");
				break;
			}
		}
		if(result)
		{
			delete new_edl;
			delete new_asset;
			new_edl = 0;
			new_asset = 0;
		}
	}

	gui->statusbar->default_message();

	mainindexes->start_build();
	master_edl->check_master_track();
	assetlist_global.remove_unused();
	update_project(oloadmode);

	undo->update_undo(_("load"), LOAD_ALL, 0);
	gui->stop_hourglass();
}

void MWindow::test_plugins(EDL *new_edl, const char *path)
{
// Do a check weather plugins exist
	for(Track *track = new_edl->first_track(); track; track = track->next)
	{
		for(int k = 0; k < track->plugin_set.total; k++)
		{
			PluginSet *plugin_set = track->plugin_set.values[k];
			for(Plugin *plugin = (Plugin*)plugin_set->first; 
				plugin;
				plugin = (Plugin*)plugin->next)
			{
				if(plugin->plugin_type == PLUGIN_STANDALONE)
				{
					// Replace old plugin name with new for compatipility with previous version
					const char *nnp = PluginServer::translate_pluginname(track->data_type, plugin->title);
					if(nnp)
						strcpy(plugin->title, nnp);
					// ok we need to find it in plugindb
					int plugin_found = 0;
					for(int j = 0; j < plugindb->total; j++)
					{
						PluginServer *server = plugindb->values[j];
						if(!strcasecmp(server->title, plugin->title) &&
							((track->data_type == TRACK_AUDIO && server->audio) ||
							(track->data_type == TRACK_VIDEO && server->video)) &&
							(!server->transition))
							plugin_found = 1;
					}
					if (!plugin_found) 
						errormsg("The effect '%s' in file '%s' is not part of your installation of Cinelerra.\n"
							"The project won't be rendered as it was meant and Cinelerra might crash.\n",
							plugin->title,
							path);
				}
			}
		}
		for(Edit *edit = (Edit*)track->edits->first;
			edit;
			edit = (Edit*)edit->next)
		{
			if (edit->transition)
			{
				// ok we need to find transition in plugindb
				int transition_found = 0;
				for(int j = 0; j < plugindb->total; j++)
				{
					PluginServer *server = plugindb->values[j];
					if(!strcasecmp(server->title, edit->transition->title) &&
						((track->data_type == TRACK_AUDIO && server->audio) ||
						(track->data_type == TRACK_VIDEO && server->video)) &&
						(server->transition))
						transition_found = 1;
				}
				if (!transition_found) 
					errormsg("The transition '%s' in file '%s' is not part of your installation of Cinelerra.\n"
						"The project won't be rendered as it was meant and Cinelerra might crash.\n",
						edit->transition->title, 
						path); 
			}
		}
	}
}

void MWindow::show_splash()
{
#include "data/heroine_logo12_png.h"
	VFrame *frame = new VFrame(heroine_logo12_png);
	int root_w, root_h;

	BC_Resources::get_root_size(&root_w, &root_h);
	splash_window = new SplashGUI(frame, 
		root_w / 2 - frame->get_w() / 2,
		root_h / 2 - frame->get_h() / 2);
	delete frame;
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
	ruler->start();
	gwindow->start();
	Thread::start();
	glthread->run();
}

void MWindow::run()
{
	gui->run_window();
}

void MWindow::show_vwindow()
{
	mainsession->show_vwindow = 1;
	vwindow->gui->show_window();
	vwindow->gui->raise_window();
	vwindow->gui->flush();
	gui->mainmenu->show_vwindow->set_checked(1);
}

void MWindow::show_awindow()
{
	mainsession->show_awindow = 1;
	awindow->gui->show_window();
	awindow->gui->raise_window();
	awindow->gui->flush();
	gui->mainmenu->show_awindow->set_checked(1);
}

void MWindow::show_cwindow()
{
	mainsession->show_cwindow = 1;
	cwindow->show_window();
	gui->mainmenu->show_cwindow->set_checked(1);
}

void MWindow::show_gwindow()
{
	mainsession->show_gwindow = 1;

	gwindow->gui->show_window();
	gwindow->gui->raise_window();
	gwindow->gui->flush();

	gui->mainmenu->show_gwindow->set_checked(1);
}

void MWindow::show_lwindow()
{
	mainsession->show_lwindow = 1;
	lwindow->gui->show_window();
	lwindow->gui->raise_window();
	lwindow->gui->flush();
	gui->mainmenu->show_lwindow->set_checked(1);
}

void MWindow::tile_windows()
{
	mainsession->default_window_positions();
	gui->default_positions();
}

void MWindow::show_ruler()
{
	mainsession->show_ruler = 1;
	ruler->gui->show_window();
	ruler->gui->raise_window();
	ruler->gui->flush();
	gui->mainmenu->show_ruler->set_checked(1);
}

void MWindow::toggle_loop_playback()
{
	master_edl->local_session->loop_playback =
		!master_edl->local_session->loop_playback;
	set_loop_boundaries();
	save_backup();

	gui->canvas->draw_overlays();
	gui->canvas->flash();
	sync_parameters(CHANGE_PARAMS);
}

void MWindow::set_titles(int value)
{
	edlsession->show_titles = value;
	trackmovement(master_edl->local_session->track_start);
}

void MWindow::set_auto_keyframes(int value)
{
	edlsession->auto_keyframes = value;
	gui->mbuttons->edit_panel->keyframe->update(value);
	gui->flush();
	cwindow->gui->edit_panel->keyframe->update(value);
	cwindow->gui->update_tool();
	cwindow->gui->flush();
}

void MWindow::set_editing_mode(int new_editing_mode)
{
	edlsession->editing_mode = new_editing_mode;
	gui->mbuttons->edit_panel->update();
	gui->canvas->update_cursor();
	cwindow->gui->edit_panel->update();
}

void MWindow::toggle_editing_mode()
{
	int mode = edlsession->editing_mode;
	if( mode == EDITING_ARROW )
		set_editing_mode(EDITING_IBEAM);
	else
		set_editing_mode(EDITING_ARROW);
}

void MWindow::set_labels_follow_edits(int value)
{
	edlsession->labels_follow_edits = value;
	gui->mbuttons->edit_panel->locklabels->update(value);
	gui->mainmenu->labels_follow_edits->set_checked(value);
	gui->flush();
}

void MWindow::sync_parameters(int change_type)
{
// Sync engines which are playing back
	if(cwindow->playback_engine->is_playing_back)
	{
		if(change_type & CHANGE_PARAMS)
		{
// TODO: block keyframes until synchronization is done
			cwindow->playback_engine->sync_parameters(master_edl);
		}
		else
// Stop and restart
		{
			int command = cwindow->playback_engine->command->command;
			cwindow->playback_engine->send_command(STOP);

// Waiting for tracking to finish would make the restart position more
// accurate but it can't lock the window to stop tracking for some reason.
// Not waiting for tracking gives a faster response but restart position is
// only as accurate as the last tracking update.

			cwindow->playback_engine->send_command(command, master_edl, change_type);
		}
	}
	else
		cwindow->playback_engine->send_command(CURRENT_FRAME, master_edl, change_type);
}

void MWindow::age_caches()
{
	size_t prev_memory_usage;
	size_t memory_usage;
	int result = 0;
	do
	{
		memory_usage = audio_cache->get_memory_usage(1) +
			video_cache->get_memory_usage(1) +
			frame_cache->get_memory_usage() +
			wave_cache->get_memory_usage();

		if(memory_usage > preferences->cache_size)
		{
			int target = 1;
			int oldest1 = audio_cache->get_oldest();
			int oldest2 = video_cache->get_oldest();
			if(oldest2 < oldest1) target = 2;
			int oldest3 = frame_cache->get_oldest();
			if(oldest3 < oldest1 && oldest3 < oldest2) target = 3;
			int oldest4 = wave_cache->get_oldest();
			if(oldest4 < oldest3 && oldest4 < oldest2 && oldest4 < oldest1) target = 4;
			switch(target)
			{
				case 1: audio_cache->delete_oldest(); break;
				case 2: video_cache->delete_oldest(); break;
				case 3: frame_cache->delete_oldest(); break;
				case 4: wave_cache->delete_oldest(); break;
			}
		}
		prev_memory_usage = memory_usage;
		memory_usage = audio_cache->get_memory_usage(1) +
			video_cache->get_memory_usage(1) +
			frame_cache->get_memory_usage() +
			wave_cache->get_memory_usage();
	} while(!result && 
		prev_memory_usage != memory_usage && 
		memory_usage > preferences->cache_size);
}

void MWindow::show_plugin(Plugin *plugin)
{
	int done = 0;

	plugin_gui_lock->lock("MWindow::show_plugin");
	removed_guis->remove_all_objects();
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

	if(!done)
	{
		PluginServer *server = scan_plugindb(plugin->title,
			plugin->track->data_type);

		if(server && server->uses_gui)
		{
			PluginServer *gui = plugin_guis->append(new PluginServer(*server));
// Needs mwindow to do GUI
			gui->set_mwindow(this);
			gui->open_plugin(0, preferences, master_edl, plugin);
			gui->show_gui();
			plugin->show = 1;
		}
	}
	plugin_gui_lock->unlock();
}

void MWindow::hide_plugin(Plugin *plugin, int lock)
{
	plugin->show = 0;
	gui->update(WUPD_CANVINCR);

	if(lock) plugin_gui_lock->lock("MWindow::hide_plugin");
	for(int i = 0; i < plugin_guis->total; i++)
	{
		if(plugin_guis->values[i]->plugin == plugin)
		{
			PluginServer *ptr = plugin_guis->values[i];
			plugin_guis->remove(ptr);
			ptr->hide_gui();
			removed_guis->append(ptr);
			if(lock) plugin_gui_lock->unlock();
			return;
		}
	}
	if(lock) plugin_gui_lock->unlock();

}

void MWindow::hide_plugins()
{
	plugin_gui_lock->lock("MWindow::hide_plugins");
	plugin_guis->remove_all_objects();
	removed_guis->remove_all_objects();
	plugin_gui_lock->unlock();
}

void MWindow::update_plugin_guis()
{
	plugin_gui_lock->lock("MWindow::update_plugin_guis");

	for(int i = 0; i < plugin_guis->total; i++)
	{
		plugin_guis->values[i]->update_gui();
	}
	plugin_gui_lock->unlock();
}

int MWindow::plugin_gui_open(Plugin *plugin)
{
	int result = 0;
	plugin_gui_lock->lock("MWindow::plugin_gui_open");
	for(int i = 0; i < plugin_guis->total; i++)
	{
		if(plugin_guis->values[i]->plugin->identical_location(plugin))
		{
			result = 1;
			break;
		}
	}
	plugin_gui_lock->unlock();
	return result;
}

void MWindow::render_plugin_gui(void *data, Plugin *plugin)
{
	plugin_gui_lock->lock("MWindow::render_plugin_gui");
	for(int i = 0; i < plugin_guis->total; i++)
	{
		if(plugin_guis->values[i]->plugin->identical_location(plugin))
		{
			plugin_guis->values[i]->render_gui(data);
			break;
		}
	}
	plugin_messages.add_msg(data, plugin);
	plugin_gui_lock->unlock();
}

void MWindow::get_gui_data(PluginServer *srv)
{
	struct pluginmsg *msg;

	if(!srv->plugin)
		return;
	plugin_gui_lock->lock("MWindow::get_gui_data");
	if(msg = plugin_messages.find_msg(srv->plugin))
		srv->render_gui(msg->data);
	plugin_gui_lock->unlock();
}

void MWindow::clear_msgs(Plugin *plugin)
{
	plugin_gui_lock->lock("MWindow::clear_msgs");
	plugin_messages.delete_msg(plugin);
	plugin_gui_lock->unlock();
}

void MWindow::update_plugin_states()
{
	plugin_gui_lock->lock("MWindow::update_plugin_states");
	for(int i = 0; i < plugin_guis->total; i++)
	{
		int result = 0;
// Get a plugin GUI
		Plugin *src_plugin = plugin_guis->values[i]->plugin;
		PluginServer *src_plugingui = plugin_guis->values[i];

// Search for plugin in EDL.  Only the master EDL shows plugin GUIs.
		for(Track *track = master_edl->first_track();
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
	removed_guis->remove_all_objects();
	plugin_gui_lock->unlock();
}

void MWindow::update_plugin_titles()
{
	for(int i = 0; i < plugin_guis->total; i++)
	{
		plugin_guis->values[i]->update_title();
	}
}

// Reset everything after a load.
void MWindow::update_project(int load_mode)
{
	restart_brender();
	master_edl->tracks->update_y_pixels(theme);

	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK | WUPD_BUTTONBAR);

	cwindow->update(WUPD_TOOLWIN | WUPD_OPERATION | WUPD_TIMEBAR | WUPD_OVERLAYS);

	if(load_mode == LOADMODE_REPLACE ||
		load_mode == LOADMODE_REPLACE_CONCATENATE)
	{
		vwindow->change_source();
	}

	cwindow->gui->slider->set_position();
	cwindow->playback_engine->send_command(CURRENT_FRAME, master_edl, CHANGE_ALL);

	awindow->gui->async_update_assets();

	gui->flush();
}

void MWindow::rebuild_indices()
{
	char source_filename[BCTEXTLEN], index_filename[BCTEXTLEN];

	for(int i = 0; i < mainsession->drag_assets->total; i++)
	{
// Erase file
		IndexFile::get_index_filename(source_filename, 
			preferences->index_directory,
			index_filename, 
			mainsession->drag_assets->values[i]->path,
			mainsession->drag_assets->values[i]->audio_streamno - 1);
		remove(index_filename);
// Schedule index build
		mainsession->drag_assets->values[i]->index_status = INDEX_NOTTESTED;
		mainindexes->add_next_asset(mainsession->drag_assets->values[i]);
	}
	mainindexes->start_build();
}

void MWindow::save_backup(int is_manual)
{
	FileXML file;
	char path[BCTEXTLEN];
	FileSystem fs;

	if(!is_manual)
	{
		if(edlsession->automatic_backups)
		{
			time_t moment = time(0);
			if(last_backup_time + edlsession->backup_interval > moment)
				return;
			last_backup_time = moment;
		}
		else
			return;
	}
	master_edl->set_project_path(mainsession->filename);
	master_edl->save_xml(&file, BACKUP_PATH, 0, 0);
	file.terminate_string();
	strcpy(path, BACKUP_PATH);
	fs.complete_path(path);

	if(file.write_to_file(path))
		gui->show_message(_("Couldn't open %s for writing."), path);
}

void MWindow::reset_caches()
{
	frame_cache->remove_all();
	wave_cache->remove_all();
	audio_cache->remove_all();
	video_cache->remove_all();
	if(cwindow->playback_engine && cwindow->playback_engine->audio_cache)
		cwindow->playback_engine->audio_cache->remove_all();
	if(cwindow->playback_engine && cwindow->playback_engine->video_cache)
		cwindow->playback_engine->video_cache->remove_all();
	if(vwindow->playback_engine && vwindow->playback_engine->audio_cache)
		vwindow->playback_engine->audio_cache->remove_all();
	if(vwindow->playback_engine && vwindow->playback_engine->video_cache)
		vwindow->playback_engine->video_cache->remove_all();
	BC_Resources::tmpframes.delete_unused();
}

void MWindow::remove_asset_from_caches(Asset *asset)
{
	frame_cache->remove_asset(asset);
	wave_cache->remove_asset(asset);
	audio_cache->delete_entry(asset);
	video_cache->delete_entry(asset);
	if(cwindow->playback_engine && cwindow->playback_engine->audio_cache)
		cwindow->playback_engine->audio_cache->delete_entry(asset);
	if(cwindow->playback_engine && cwindow->playback_engine->video_cache)
		cwindow->playback_engine->video_cache->delete_entry(asset);
	if(vwindow->playback_engine && vwindow->playback_engine->audio_cache)
		vwindow->playback_engine->audio_cache->delete_entry(asset);
	if(vwindow->playback_engine && vwindow->playback_engine->video_cache)
		vwindow->playback_engine->video_cache->delete_entry(asset);
}

void MWindow::remove_assets_from_project(int push_undo)
{
	for(int i = 0; i < mainsession->drag_assets->total; i++)
	{
		Asset *asset = mainsession->drag_assets->values[i];
		remove_asset_from_caches(asset);
	}

// Remove from VWindow.
	for(int i = 0; i < mainsession->drag_clips->total; i++)
	{
		if(mainsession->drag_clips->values[i] == vwindow_edl)
			vwindow->remove_source();
	}

	for(int i = 0; i < mainsession->drag_assets->total; i++)
	{
		if(mainsession->drag_assets->values[i] == vwindow->get_asset())
			vwindow->remove_source();
	}

	master_edl->remove_from_project(mainsession->drag_assets);
	cliplist_global.remove_from_project(mainsession->drag_clips);
	save_backup();
	if(push_undo) undo->update_undo(_("remove assets"), LOAD_ALL);
	restart_brender();

	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_CLOCK);

	awindow->gui->async_update_assets();

// Removes from playback here
	sync_parameters(CHANGE_ALL);
}

void MWindow::save_defaults()
{
	gui->save_defaults(defaults);
	master_edl->save_defaults(defaults, edlsession);
	mainsession->save_defaults(defaults);
	preferences->save_defaults(defaults);
// Remove old defaults
// Channel
	defaults->delete_key("SCAN_FREQTABLE");
	defaults->delete_key("SCAN_INPUT");
	defaults->delete_key("SCAN_NORM");
// Record
	defaults->delete_key("TOTAL_BATCHES");
	defaults->delete_keys_prefix("RECORD_");
	defaults->delete_keys_prefix("BATCH_ENABLED_");
	defaults->delete_key("REVERSE_INTERLACE");
	defaults->delete_key("FILL_DROPPED_FRAMES");
	defaults->delete_keys_prefix("BATCH_COLUMNWIDTH_");
// PictureConfig
	defaults->delete_key("VIDEO_BRIGHTNESS");
	defaults->delete_key("VIDEO_HUE");
	defaults->delete_key("VIDEO_COLOR");
	defaults->delete_key("VIDEO_CONTRAST");
	defaults->delete_key("VIDEO_WHITENESS");
// AudioInConfig
// Playbackconfig uses OSS_ENABLE_n_m - can't use delete_keys_prefix
	defaults->delete_key("OSS_ENABLE_0");
	defaults->delete_key("OSS_ENABLE_1");
	defaults->delete_key("OSS_ENABLE_2");
	defaults->delete_key("OSS_ENABLE_3");
	defaults->delete_key("OSS_ENABLE_4");
	defaults->delete_key("OSS_ENABLE_5");
	defaults->delete_key("OSS_ENABLE_6");
	defaults->delete_key("OSS_ENABLE_7");
	defaults->delete_keys_prefix("OSS_IN_DEVICE_");
	defaults->delete_key("OSS_IN_BITS");
	defaults->delete_key("ESOUND_IN_SERVER");
	defaults->delete_key("ESOUND_IN_PORT");
	defaults->delete_key("ALSA_IN_DEVICE");
	defaults->delete_key("ALSA_IN_BITS");
	defaults->delete_key("IN_SAMPLERATE");
	defaults->delete_key("IN_CHANNELS");
// VideoInConfig
	defaults->delete_key("VIDEO_IN_DRIVER");
	defaults->delete_key("V4L_IN_DEVICE");
	defaults->delete_key("V4L2_IN_DEVICE");
	defaults->delete_key("V4L2JPEG_IN_DEVICE");
	defaults->delete_key("SCREENCAPTURE_DISPLAY");
	defaults->delete_key("DVB_IN_HOST");
	defaults->delete_key("DVB_IN_PORT");
	defaults->delete_key("DVB_IN_NUMBER");
	defaults->delete_key("VIDEO_CAPTURE_LENGTH");
	defaults->delete_key("IN_FRAMERATE");
	defaults->save();
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
				errorbox("Unrecognized command: '%s' in script",script->tag.get_title() );
			}
		}
	}
	return result2;
}

// ================================= synchronization

void MWindow::interrupt_indexes()
{
	mainindexes->interrupt_build();
}

void MWindow::next_time_format()
{
	switch(edlsession->time_format)
	{
	case TIME_HMS:
		edlsession->time_format = TIME_HMSF;
		break;
	case TIME_HMSF:
		edlsession->time_format = TIME_SAMPLES;
		break;
	case TIME_SAMPLES:
		edlsession->time_format = TIME_SAMPLES_HEX;
		break;
	case TIME_SAMPLES_HEX: 
		edlsession->time_format = TIME_FRAMES;
		break;
	case TIME_FRAMES:
		edlsession->time_format = TIME_FEET_FRAMES;
		break;
	case TIME_FEET_FRAMES:
		edlsession->time_format = TIME_SECONDS;
		break;
	case TIME_SECONDS:
		edlsession->time_format = TIME_HMS;
		break;
	}

	time_format_common();
}

void MWindow::prev_time_format()
{
	switch(edlsession->time_format)
	{
	case TIME_HMS:
		edlsession->time_format = TIME_SECONDS;
		break;
	case TIME_SECONDS:
		edlsession->time_format = TIME_FEET_FRAMES;
		break;
	case TIME_FEET_FRAMES:
		edlsession->time_format = TIME_FRAMES;
		break;
	case TIME_FRAMES:
		edlsession->time_format = TIME_SAMPLES_HEX;
		break;
	case TIME_SAMPLES_HEX:
		edlsession->time_format = TIME_SAMPLES;
		break;
	case TIME_SAMPLES:
		edlsession->time_format = TIME_HMSF;
		break;
	case TIME_HMSF:
		edlsession->time_format = TIME_HMS;
		break;
	}

	time_format_common();
}

void MWindow::time_format_common()
{
	gui->redraw_time_dependancies();
	char string[BCTEXTLEN];
	gui->show_message(_("Using %s."), Units::print_time_format(edlsession->time_format, string));
	gui->flush();
}


void MWindow::set_filename(const char *filename)
{
	strcpy(mainsession->filename, filename);
	if(gui)
	{
		if(filename[0] == 0)
		{
			gui->set_title(PROGRAM_NAME);
		}
		else
		{
			FileSystem dir;
			char string[BCTEXTLEN];
			dir.extract_name(string, filename);
			if(BCTEXTLEN - strlen(string) > 64)
				strcat(string, " - " PROGRAM_NAME);
			gui->set_title(string);
		}
	}
}

void MWindow::set_loop_boundaries()
{
	ptstime start = master_edl->local_session->get_selectionstart();
	ptstime end = master_edl->local_session->get_selectionend();

	if(!PTSEQU(start, end))
	{
		;
	}
	else
	if(master_edl->total_length())
	{
		start = 0;
		end = master_edl->total_length();
	}
	else
	{
		start = end = 0;
	}

	if(master_edl->local_session->loop_playback && !PTSEQU(start, end))
	{
		master_edl->local_session->loop_start = start;
		master_edl->local_session->loop_end = end;
	}
}

void MWindow::reset_meters()
{
	cwindow->gui->meters->reset_meters();
	vwindow->gui->meters->reset_meters();
	lwindow->gui->panel->reset_meters();
	gui->patchbay->reset_meters();
}

void MWindow::show_program_status()
{
	size_t mc, cc, vc;

	printf("%s status:\n", version_name);
	if(master_edl)
	{
		printf(" Audio channels %d tracks %d samplerate %d\n",
			edlsession->audio_channels, edlsession->audio_tracks,
			edlsession->sample_rate);
		printf( " Video [%d,%d] channels %d tracks %d framerate %.2f '%s'\n",
			edlsession->output_w,
			edlsession->output_h,
			edlsession->video_channels,
			edlsession->video_tracks,
			edlsession->frame_rate,
			ColorModels::name(edlsession->color_model));
	}
	else
		printf(" No edl\n");
	printf(" Internal encoding: '%s'\n", BC_Resources::encoding);
	mc = audio_cache->get_memory_usage(1);
	vc = cc = 0;
	if(vwindow->playback_engine && vwindow->playback_engine->audio_cache)
		vc = vwindow->playback_engine->audio_cache->get_memory_usage(1);
	if(cwindow->playback_engine && cwindow->playback_engine->audio_cache)
		cc = cwindow->playback_engine->audio_cache->get_memory_usage(1);
	printf(" Audio cache %zu %zu %zu = %zu\n", mc, vc, cc, mc + vc + cc);
	mc = video_cache->get_memory_usage(1);
	vc = cc = 0;
	if(vwindow->playback_engine && vwindow->playback_engine->video_cache)
		vc = vwindow->playback_engine->video_cache->get_memory_usage(1);
	if(cwindow->playback_engine && cwindow->playback_engine->video_cache)
		vc = cwindow->playback_engine->video_cache->get_memory_usage(1);
	printf(" Video cache %zu %zu %zu = %zu\n", mc, vc, cc, mc + vc + cc);
	printf(" Frame cache %zu\n", frame_cache->get_memory_usage());
	printf(" Wave cahce %zu\n", wave_cache->get_memory_usage());
	printf(" Tmpframes %zuk\n", BC_Resources::tmpframes.get_size());
	printf(" Output device: %s\n",
		VDriverMenu::driver_to_string(edlsession->playback_config->vconfig->driver));
	if(edlsession->playback_config->vconfig->driver == PLAYBACK_X11_GL)
	{
		const char **vs = BC_Resources::OpenGLStrings;
		if(vs[0])
			printf("  OpenGl version: %s\n", vs[0]);
		if(vs[1])
			printf("  OpenGl vendor: %s\n", vs[1]);
		if(vs[2])
			printf("  OpenGl renderer: %s\n", vs[2]);
	}
}

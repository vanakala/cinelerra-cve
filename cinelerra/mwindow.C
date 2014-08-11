
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
#include "assets.h"
#include "awindowgui.h"
#include "awindow.h"
#include "batchrender.h"
#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "brender.h"
#include "cache.h"
#include "channel.h"
#include "channeldb.h"
#include "cinelerra.h"
#include "clip.h"
#include "colormodels.h"
#include "cplayback.h"
#include "ctimebar.h"
#include "cwindowgui.h"
#include "cwindow.h"
#include "bchash.h"
#include "devicedvbinput.inc"
#include "editpanel.h"
#include "edl.h"
#include "edlsession.h"
#include "fileformat.h"
#include "file.h"
#include "filesystem.h"
#include "filexml.h"
#include "framecache.h"
#include "gwindow.h"
#include "gwindowgui.h"
#include "indexfile.h"
#include "interlacemodes.h"
#include "language.h"
#include "levelwindowgui.h"
#include "levelwindow.h"
#include "loadfile.inc"
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
#include "playback3d.h"
#include "playbackengine.h"
#include "plugin.h"
#include "pluginserver.h"
#include "pluginset.h"
#include "preferences.h"
#include "record.h"
#include "recordlabel.h"
#include "removethread.h"
#include "render.h"
#include "ruler.h"
#include "samplescroll.h"
#include "sighandler.h"
#include "splashgui.h"
#include "statusbar.h"
#include "theme.h"
#include "timebar.h"
#include "tipwindow.h"
#include "trackcanvas.h"
#include "track.h"
#include "tracking.h"
#include "tracks.h"
#include "transition.h"
#include "transportcommand.h"
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

extern Theme *theme_global;

MWindow::MWindow()
 : Thread(THREAD_SYNCHRONOUS)
{
	plugin_gui_lock = new Mutex("MWindow::plugin_gui_lock");
	brender_lock = new Mutex("MWindow::brender_lock");
	brender = 0;
	session = 0;
	channeldb_v4l2jpeg = new ChannelDB;
	dvb_input = 0;
	dvb_input_lock = new Mutex("MWindow::dvb_input_lock");
}

MWindow::~MWindow()
{
	brender_lock->lock("MWindow::~MWindow");
	if(brender) delete brender;
	brender = 0;
	brender_lock->unlock();
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
	delete plugin_guis;
	delete plugin_gui_lock;
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
				int result = new_plugin->open_plugin(1, preferences, 0, 0, -1);

				if(!result)
				{
					new_plugin->save_data(&new_plugin->default_keyframe);
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
	session = new MainSession(this);
	session->load_defaults(defaults);
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
	fs.set_filter("*.idx");
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
				strcpy(ptr, ".toc");
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
	gwindow->create_objects();
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
			plugin.open_plugin(0, preferences, 0, 0, -1);
			theme = plugin.new_theme();
			theme->mwindow = this;
			strcpy(theme->path, plugin.path);
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
				plugin.open_plugin(0, preferences, 0, 0, -1);
				theme = plugin.new_theme();
				theme->mwindow = this;
				strcpy(theme->path, plugin.path);
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
}

void MWindow::init_3d()
{
	playback_3d = new Playback3D(this);
}

void MWindow::init_edl()
{
	edl = new EDL;
	FormatPresets::fill_preset_defaults(default_standard, edl->session);
	edl->load_defaults(defaults);
	edl->create_default_tracks();
	edl->tracks->update_y_pixels(theme);
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
	audio_cache = new CICache(preferences, plugindb);
	video_cache = new CICache(preferences, plugindb);
	frame_cache = new FrameCache;
	wave_cache = new WaveCache;
}

void MWindow::init_ruler()
{
	ruler = new Ruler(this);
}

void MWindow::init_channeldb()
{
	channeldb_v4l2jpeg->load("channeldb_v4l2jpeg");
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
	sighandler->initXErrors();
	ENABLE_BUFFER
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
	if(brender) brender->restart(edl);
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
	edl->session->brender_start = edl->local_session->get_selectionstart();
	restart_brender();
	gui->canvas->draw_overlays();
	gui->canvas->flash();
}

void MWindow::load_filenames(ArrayList<char*> *filenames, 
	int load_mode,
	int update_filename,
	const char *reel_name,
	int reel_number,
	int overwrite_reel)
{
SET_TRACE
	ArrayList<EDL*> new_edls;
	ArrayList<Asset*> new_assets;
	ArrayList<File*> new_files;

	save_defaults();
	gui->start_hourglass();

// Need to stop playback since tracking depends on the EDL not getting
// deleted.
	cwindow->playback_engine->send_command(STOP);
	vwindow->playback_engine->send_command(STOP);

// Define new_edls and new_assets to load
	int result = 0;
	for(int i = 0; i < filenames->total; i++)
	{
// Get type of file
		File *new_file = new File;
		Asset *new_asset = new Asset(filenames->values[i]);
		EDL *new_edl = new EDL;
		char string[BCTEXTLEN];

// Set reel name and number for the asset
// If the user wants to overwrite the last used reel number for the clip,
// we have to rebuild the index for the file

		if(overwrite_reel)
		{
			char source_filename[BCTEXTLEN];
			char index_filename[BCTEXTLEN];

			strcpy(new_asset->reel_name, reel_name);
			new_asset->reel_number = reel_number;

			IndexFile::get_index_filename(source_filename,
				preferences->index_directory,
				index_filename,
				new_asset->path);
			remove(index_filename);
			new_asset->index_status = INDEX_NOTTESTED;
		}
		new_edl->copy_session(edl);

		gui->show_message("Loading %s", new_asset->path);
		result = new_file->open_file(preferences, new_asset, 1, 0, 0, 0);

		switch(result)
		{
// Convert media file to EDL
			case FILE_OK:
// Warn about odd image dimensions
				if(new_asset->video_data &&
					((new_asset->width % 2) ||
					(new_asset->height % 2)))
				{
					errormsg("%s's\nresolution is %dx%d. Images with odd dimensions may not decode properly.",
						new_asset->path,
						new_asset->width,
						new_asset->height);
				}

				if(load_mode != LOADMODE_RESOURCESONLY)
				{
					asset_to_edl(new_edl, new_asset);
					new_edls.append(new_edl);
					Garbage::delete_object(new_asset);
					new_asset = 0;
				}
				else
				{
					new_assets.append(new_asset);
				}

// Set filename to nothing for assets since save EDL would overwrite them.
				if(load_mode == LOADMODE_REPLACE || 
					load_mode == LOADMODE_REPLACE_CONCATENATE)
				{
					set_filename("");
// Reset timeline position
					new_edl->local_session->view_start_pts = 0;
					new_edl->local_session->track_start = 0;
				}

				result = 0;
				break;

// File not found
			case FILE_NOT_FOUND:
				errormsg(_("Failed to open %s"), new_asset->path);
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

					FileFormat fwindow(this, new_asset, string);
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
					}
				}

// Append to list
				if(!result)
				{
// Recalculate length
					delete new_file;
					new_file = new File;
					result = new_file->open_file(preferences, new_asset, 1, 0, 0, 0);

					if(load_mode != LOADMODE_RESOURCESONLY)
					{
						asset_to_edl(new_edl, new_asset);
						new_edls.append(new_edl);
						Garbage::delete_object(new_asset);
						new_asset = 0;
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
				FileXML xml_file;
				xml_file.read_from_file(filenames->values[i]);
// Load EDL for pasting
				result = 0;
				new_edl->load_xml(plugindb, &xml_file, LOAD_ALL);
				test_plugins(new_edl, filenames->values[i]);

// We don't want a valid reel name/number for projects
				strcpy(new_asset->reel_name, "");
				reel_number = -1;

				if(load_mode == LOADMODE_REPLACE || 
					load_mode == LOADMODE_REPLACE_CONCATENATE)
				{
					strcpy(session->filename, filenames->values[i]);
					strcpy(new_edl->local_session->clip_title, filenames->values[i]);
					if(update_filename)
						set_filename(new_edl->local_session->clip_title);
				}

// Open media files found in xml - open fills media info
				for(Asset *current = new_edl->assets->first; current; current = NEXT)
				{
					new_files.append(new_file);
					new_file = new File;
					if(new_file->open_file(preferences, current, 1, 0, 0, 0) != FILE_OK)
					{
						result++;
						break;
					}
				}
				if(!result)
					new_edls.append(new_edl);
				else if(update_filename)
					set_filename("");
				break;
			}
		}

		if(result)
		{
			delete new_edl;
			Garbage::delete_object(new_asset);
			new_edl = 0;
			new_asset = 0;
		}

// Store for testing index
		new_files.append(new_file);
	}

	gui->statusbar->default_message();

// Paste them.
// Don't back up here.
	if(new_edls.total)
	{
// For pasting, clear the active region
		if(load_mode == LOADMODE_PASTE)
		{
			ptstime start = edl->local_session->get_selectionstart();
			ptstime end = edl->local_session->get_selectionend();
			if(!EQUIV(start, end))
				edl->clear(start, 
					end,
					edl->session->edit_actions());
		}

		paste_edls(&new_edls, 
			load_mode,
			0,
			-1,
			edl->session->edit_actions(),
			0); // overwrite
	}

// Add new assets to EDL and schedule assets for index building.
// Used for loading resources only.
	if(new_assets.total)
	{
		for(int i = 0; i < new_assets.total; i++)
		{
			Asset *new_asset = new_assets.values[i];
			File *new_file = 0;
			File *index_file = 0;
			int got_it = 0;
			for(int j = 0; j < new_files.total; j++)
			{
				new_file = new_files.values[j];
				if(!strcmp(new_file->asset->path,
					new_asset->path))
				{
					got_it = 1;
					break;
				}
			}

			mainindexes->add_next_asset(got_it ? new_file : 0, 
				new_asset);
			edl->assets->update(new_asset);
		}

// Start examining next batch of index files
		mainindexes->start_build();
	}

	update_project(load_mode);

	new_edls.remove_all_objects();

	for(int i = 0; i < new_assets.total; i++)
		Garbage::delete_object(new_assets.values[i]);
	new_assets.remove_all();
	new_files.remove_all_objects();

	undo->update_undo(_("load"), LOAD_ALL, 0);
	gui->stop_hourglass();
}

void MWindow::test_plugins(EDL *new_edl, const char *path)
{
// Do a check weather plugins exist
	for(Track *track = new_edl->tracks->first; track; track = track->next)
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

#define FONT_SEARCHPATH "fonts"

void MWindow::create_objects(int want_gui, 
	int want_new,
	char *config_path)
{
	char string[BCTEXTLEN];
	edl = 0;
	init_signals();

	init_3d();
	remove_thread = new RemoveThread;
	remove_thread->create_objects();
	show_splash();

	init_defaults(defaults, config_path);
	default_standard = default_std();
	init_preferences();
	init_plugins(preferences, plugindb, splash_window);
	if(splash_window) splash_window->operation->update(_("Initializing GUI"));
	init_theme();
	init_error();

	strcpy(string, preferences->global_plugin_dir);
	strcat(string, "/" FONT_SEARCHPATH);
	BC_Resources::init_fontconfig(string);

// Default project created here
	init_edl();

	init_awindow();
	init_compositor();
	init_levelwindow();
	init_viewer();
	init_ruler();
	init_cache();
	init_indexes();
	init_channeldb();

	init_gui();
	init_gwindow();

	init_render();
	init_brender();
	init_exportedl();
	mainprogress = new MainProgress(this, gui);
	undo = new MainUndo(this);

	plugin_guis = new ArrayList<PluginServer*>;

	if(session->show_vwindow) vwindow->gui->show_window();
	if(session->show_cwindow) cwindow->gui->show_window();
	if(session->show_awindow) awindow->gui->show_window();
	if(session->show_lwindow) lwindow->gui->show_window();
	if(session->show_gwindow) gwindow->gui->show_window();
	if(session->show_ruler) ruler->gui->show_window();

	gui->mainmenu->load_defaults(defaults);
	gui->mainmenu->update_toggles(0);
	gui->patchbay->update();
	gui->canvas->draw();
	gui->cursor->draw(1);
	gui->show_window();
	gui->raise_window();

	if(preferences->use_tipwindow)
		init_tipwindow();

	hide_splash();
}

void MWindow::show_splash()
{
#include "data/heroine_logo12_png.h"
	VFrame *frame = new VFrame(heroine_logo12_png);
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
ENABLE_BUFFER
	vwindow->start();
	awindow->start();
	cwindow->start();
	lwindow->start();
	ruler->start();
	gwindow->start();
	Thread::start();
	playback_3d->start();
}

void MWindow::run()
{
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
	cwindow->show_window();
	gui->mainmenu->show_cwindow->set_checked(1);
}

void MWindow::show_gwindow()
{
	session->show_gwindow = 1;

	gwindow->gui->lock_window("MWindow::show_gwindow");
	gwindow->gui->show_window();
	gwindow->gui->raise_window();
	gwindow->gui->flush();
	gwindow->gui->unlock_window();

	gui->mainmenu->show_gwindow->set_checked(1);
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

void MWindow::show_ruler()
{
	session->show_ruler = 1;
	ruler->gui->show_window();
	ruler->gui->raise_window();
	ruler->gui->flush();
	gui->mainmenu->show_ruler->set_checked(1);
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

void MWindow::set_editing_mode(int new_editing_mode)
{
	gui->lock_window("MWindow::set_editing_mode");
	edl->session->editing_mode = new_editing_mode;
	gui->mbuttons->edit_panel->update();
	gui->canvas->update_cursor();
	gui->unlock_window();
	cwindow->gui->lock_window("MWindow::set_editing_mode");
	cwindow->gui->edit_panel->update();
	cwindow->gui->unlock_window();
}

void MWindow::toggle_editing_mode()
{
	int mode = edl->session->editing_mode;
	if( mode == EDITING_ARROW )
		set_editing_mode(EDITING_IBEAM);
	else
		set_editing_mode(EDITING_ARROW);
}

void MWindow::set_labels_follow_edits(int value)
{
	gui->lock_window("MWindow::set_labels_follow_edits");
	edl->session->labels_follow_edits = value;
	gui->mbuttons->edit_panel->locklabels->update(value);
	gui->mainmenu->labels_follow_edits->set_checked(value);
	gui->flush();
	gui->unlock_window();
}

void MWindow::sync_parameters(int change_type)
{
// Sync engines which are playing back
	if(cwindow->playback_engine->is_playing_back)
	{
		if(change_type & CHANGE_PARAMS)
		{
// TODO: block keyframes until synchronization is done
			cwindow->playback_engine->sync_parameters(edl);
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

			cwindow->playback_engine->send_command(command, edl, change_type);
		}
	}
	else
		cwindow->playback_engine->send_command(CURRENT_FRAME, edl, change_type);
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
			gui->open_plugin(0, preferences, edl, plugin, -1);
			gui->show_gui();
			plugin->show = 1;
		}
	}
	plugin_gui_lock->unlock();
}

void MWindow::hide_plugin(Plugin *plugin, int lock)
{
	plugin->show = 0;
	gui->lock_window("MWindow::hide_plugin");
	gui->update(WUPD_CANVINCR);
	gui->unlock_window();

	if(lock) plugin_gui_lock->lock("MWindow::hide_plugin");
	for(int i = 0; i < plugin_guis->total; i++)
	{
		if(plugin_guis->values[i]->plugin == plugin)
		{
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
	plugin_gui_lock->lock("MWindow::hide_plugins");
	plugin_guis->remove_all_objects();
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
	plugin_gui_lock->unlock();
}

void MWindow::render_plugin_gui(void *data, int size, Plugin *plugin)
{
	plugin_gui_lock->lock("MWindow::render_plugin_gui");
	for(int i = 0; i < plugin_guis->total; i++)
	{
		if(plugin_guis->values[i]->plugin->identical_location(plugin))
		{
			plugin_guis->values[i]->render_gui(data, size);
			break;
		}
	}
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

void MWindow::asset_to_edl(EDL *new_edl, 
	Asset *new_asset, 
	RecordLabels *labels)
{
// Keep frame rate, sample rate, and output size unchanged.
// These parameters would revert the project if VWindow displayed an asset
// of different size than the project.
	if(new_asset->video_data)
	{
		new_edl->session->video_tracks = new_asset->layers;
	}
	else
		new_edl->session->video_tracks = 0;

	if(new_asset->audio_data)
		new_edl->session->audio_tracks = new_asset->channels;
	else
		new_edl->session->audio_tracks = 0;

	new_edl->create_default_tracks();

	new_edl->insert_asset(new_asset, 0, 0, labels);

// Align cursor on frames:: clip the new_edl to the minimum of the last joint frame.
	if(edl->session->cursor_on_frames)
	{
		ptstime edl_length = new_edl->tracks->total_length_framealigned(edl->session->frame_rate);
		new_edl->tracks->clear(edl_length, new_edl->tracks->total_length() + 100, 1);
	}

	char string[BCTEXTLEN];
	FileSystem fs;
	fs.extract_name(string, new_asset->path);

	strcpy(new_edl->local_session->clip_title, string);
}

// Reset everything after a load.
void MWindow::update_project(int load_mode)
{
	restart_brender();
	edl->tracks->update_y_pixels(theme);

	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK | WUPD_BUTTONBAR);

	cwindow->gui->lock_window("Mwindow::update_project 1");
	cwindow->update(WUPD_TOOLWIN | WUPD_OPERATION | WUPD_TIMEBAR);
	cwindow->gui->unlock_window();

	if(load_mode == LOADMODE_REPLACE ||
		load_mode == LOADMODE_REPLACE_CONCATENATE)
	{
		vwindow->change_source();
	}

	cwindow->gui->lock_window("Mwindow::update_project 2");
	cwindow->gui->slider->set_position();
	cwindow->gui->timebar->update();
	cwindow->gui->unlock_window();
	cwindow->playback_engine->send_command(CURRENT_FRAME, edl, CHANGE_ALL);

	awindow->gui->async_update_assets();

	gui->flush();
}

void MWindow::rebuild_indices()
{
	char source_filename[BCTEXTLEN], index_filename[BCTEXTLEN];

	for(int i = 0; i < session->drag_assets->total; i++)
	{
// Erase file
		IndexFile::get_index_filename(source_filename, 
			preferences->index_directory,
			index_filename, 
			session->drag_assets->values[i]->path);
		remove(index_filename);
// Schedule index build
		session->drag_assets->values[i]->index_status = INDEX_NOTTESTED;
		mainindexes->add_next_asset(0, session->drag_assets->values[i]);
	}
	mainindexes->start_build();
}

void MWindow::save_backup()
{
	FileXML file;
	edl->set_project_path(session->filename);
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
		gui->show_message(_("Couldn't open %s for writing."), path);
}

void MWindow::create_aspect_ratio(double &w, double &h, int width, int height)
{
	int denominator;

	if(!width || !height) return;

	double fraction = (double)width / height;

	for(denominator = 1; 
		denominator < 100 && 
			fabs(fraction * denominator - (int)(fraction * denominator)) > .001; 
		denominator++)
		;

	w = denominator * width / height;
	h = denominator;
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
	for(int i = 0; i < session->drag_assets->total; i++)
	{
		Asset *asset = session->drag_assets->values[i];
		remove_asset_from_caches(asset);
	}

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
	if(push_undo) undo->update_undo(_("remove assets"), LOAD_ALL);
	restart_brender();

	gui->lock_window("MWindow::remove_assets_from_project 3");
	gui->update(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_CLOCK);
	gui->unlock_window();

	awindow->gui->async_update_assets();

// Removes from playback here
	sync_parameters(CHANGE_ALL);
}

void MWindow::remove_assets_from_disk()
{
// Remove from disk
	for(int i = 0; i < session->drag_assets->total; i++)
	{
		remove_thread->remove_file(session->drag_assets->values[i]->path);
	}

	remove_assets_from_project(1);
}

void MWindow::save_defaults()
{
	gui->save_defaults(defaults);
	edl->save_defaults(defaults);
	session->save_defaults(defaults);
	preferences->save_defaults(defaults);

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
	switch(edl->session->time_format)
	{
	case TIME_HMS:
		edl->session->time_format = TIME_HMSF;
		break;
	case TIME_HMSF:
		edl->session->time_format = TIME_SAMPLES;
		break;
	case TIME_SAMPLES:
		edl->session->time_format = TIME_SAMPLES_HEX;
		break;
	case TIME_SAMPLES_HEX: 
		edl->session->time_format = TIME_FRAMES;
		break;
	case TIME_FRAMES:
		edl->session->time_format = TIME_FEET_FRAMES;
		break;
	case TIME_FEET_FRAMES:
		edl->session->time_format = TIME_SECONDS;
		break;
	case TIME_SECONDS:
		edl->session->time_format = TIME_HMS;
		break;
	}

	time_format_common();
}

void MWindow::prev_time_format()
{
	switch(edl->session->time_format)
	{
	case TIME_HMS:
		edl->session->time_format = TIME_SECONDS;
		break;
	case TIME_SECONDS:
		edl->session->time_format = TIME_FEET_FRAMES;
		break;
	case TIME_FEET_FRAMES:
		edl->session->time_format = TIME_FRAMES;
		break;
	case TIME_FRAMES:
		edl->session->time_format = TIME_SAMPLES_HEX;
		break;
	case TIME_SAMPLES_HEX:
		edl->session->time_format = TIME_SAMPLES;
		break;
	case TIME_SAMPLES:
		edl->session->time_format = TIME_HMSF;
		break;
	case TIME_HMSF:
		edl->session->time_format = TIME_HMS;
		break;
	}

	time_format_common();
}

void MWindow::time_format_common()
{
	gui->lock_window("MWindow::next_time_format");
	gui->redraw_time_dependancies();
	char string[BCTEXTLEN];
	gui->show_message(_("Using %s."), Units::print_time_format(edl->session->time_format, string));
	gui->flush();
	gui->unlock_window();
}


void MWindow::set_filename(const char *filename)
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
			char string[BCTEXTLEN];
			dir.extract_name(string, filename);
			gui->set_title(string);
		}
	}
}

void MWindow::set_loop_boundaries()
{
	ptstime start = edl->local_session->get_selectionstart();
	ptstime end = edl->local_session->get_selectionend();

	if(!PTSEQU(start, end))
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

	if(edl->local_session->loop_playback && !PTSEQU(start, end))
	{
		edl->local_session->loop_start = start;
		edl->local_session->loop_end = end;
	}
}

void MWindow::reset_meters()
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
}

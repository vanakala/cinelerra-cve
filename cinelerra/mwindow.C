// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "assetlist.h"
#include "atmpframecache.h"
#include "awindowgui.h"
#include "awindow.h"
#include "batchrender.h"
#include "bcdisplayinfo.h"
#include "bcprogress.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "brender.h"
#include "cinelerra.h"
#include "clip.h"
#include "clipedit.h"
#include "cliplist.h"
#include "colormodels.h"
#include "cplayback.h"
#include "ctimebar.h"
#include "cwindowgui.h"
#include "cwindow.h"
#include "bchash.h"
#include "edit.h"
#include "edits.h"
#include "editpanel.h"
#include "edl.h"
#include "edlsession.h"
#include "fileformat.h"
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
#include "mainclock.h"
#include "maincursor.h"
#include "mainerror.h"
#include "mainindexes.h"
#include "mainmenu.h"
#include "mainprogress.h"
#include "mainsession.h"
#include "mainundo.h"
#include "maplist.h"
#include "mbuttons.h"
#include "mtimebar.h"
#include "mutex.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "new.h"
#include "patchbay.h"
#include "playbackconfig.h"
#include "playbackengine.h"
#include "plugin.h"
#include "plugindb.h"
#include "pluginserver.h"
#include "pluginclient.h"
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
#include "transportcommand.h"
#include "vdeviceprefs.h"
#include "vframe.h"
#include "videodevice.inc"
#include "vplayback.h"
#include "vwindowgui.h"
#include "vwindow.h"
#include "zoombar.h"
#include "exportedl.h"
#include "ntsczones.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

extern const char *version_name;

MWindow::MWindow(const char *config_path)
 : Thread(THREAD_SYNCHRONOUS)
{
	char string[BCTEXTLEN];

	plugin_gui_lock = new Mutex("MWindow::plugin_gui_lock");
	brender_lock = new Mutex("MWindow::brender_lock");
	brender = 0;

	sighandler = new SigHandler;
	sighandler->initialize();
	sighandler->initXErrors();

	mwindow_global = this;

	glthread = new GLThread();

	show_splash();

	init_defaults(defaults, config_path);
	default_standard = default_std();

	master_edl = new EDL(1);
	edlsession = new EDLSession();
	vwindow_edl = new EDL(0);

	FormatPresets::fill_preset_defaults(default_standard, edlsession);
	master_edl->load_defaults(defaults, edlsession);
	master_edl->create_default_tracks();

	preferences = new Preferences;
	preferences->load_defaults(defaults);
	mainsession = new MainSession();
	mainsession->load_defaults(defaults);
	preferences_global = preferences;

	plugindb.init_plugins(splash_window);
	if(splash_window)
		splash_window->operation->update(_("Initializing GUI"));

	init_theme();
	new MainError();

	strcpy(string, preferences->global_plugin_dir);
	strcat(string, "/" FONT_SEARCHPATH);
	BC_Resources::init_fontconfig(string);
	last_backup_time = time(0);

	awindow = new AWindow;
	cwindow = new CWindow;
	lwindow = new LevelWindow;
	vwindow = new VWindow;
	ruler = new Ruler;
	mainindexes = new MainIndexes();
	mainindexes->start_loop();

	gui = new MWindowGUI();
	gui->show();
	gui->load_defaults(defaults);

	gwindow = new GWindow(gui);

	render = new Render;
	batch_render = new BatchRenderThread;

	init_brender();

	exportedl = new ExportEDL;

	mainprogress = new MainProgress(gui);
	undo = new MainUndo;
	clip_edit = new ClipEdit;

	if(mainsession->show_vwindow)
		vwindow->gui->show_window();
	if(mainsession->show_cwindow)
		cwindow->gui->show_window();
	if(mainsession->show_awindow)
		awindow->gui->show_window();
	if(mainsession->show_lwindow)
		lwindow->gui->show_window();
	if(mainsession->show_gwindow)
		gwindow->gui->show_window();
	if(mainsession->show_ruler)
		ruler->gui->show_window();

	gui->mainmenu->load_defaults(defaults);
	gui->mainmenu->update_toggles();
	gui->patchbay->update();
	gui->show_window();
	gui->raise_window();

	if(preferences->use_tipwindow)
	{
		twindow = new TipWindow();
		twindow->start();
	}
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
	delete gui;
	delete undo;
	delete clip_edit;
	delete preferences;
	delete defaults;
	delete render;
	delete awindow;
	delete vwindow;
	delete cwindow;
	delete lwindow;
	delete plugin_gui_lock;
	delete vwindow_edl;
	delete master_edl;
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

void MWindow::init_theme()
{
	PluginServer *server = plugindb.get_theme(preferences->theme);
	PluginClient *client = 0;

	if(server->theme)
		client = server->open_plugin(0, 0);

	if(!client)
	{
		errorbox(_("Can't find theme '%s'."), preferences->theme);
		// Theme load fails, try default
		strcpy(preferences->theme, DEFAULT_THEME);
		server = plugindb.get_theme(preferences->theme);
		if(server->theme)
			client = server->open_plugin(0, 0);
	}

	if(!client)
	{
		errorbox(_("Cant load default theme '%s'."), preferences->theme);
		exit(1);
	}

	theme_global = theme = client->new_theme();
	server->close_plugin(client);
	theme->mwindow = this;
// Load images which may have been forgotten
	theme->Theme::initialize();
// Load user images
	theme->initialize();

	theme->check_used();
	master_edl->tracks->update_y_pixels(theme);
}

void MWindow::init_brender()
{
	if(preferences->use_brender && !brender)
	{
		brender_lock->lock("MWindow::init_brender");
		brender = new BRender();
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

void MWindow::reset_brender()
{
	if(brender)
	{
		brender_lock->lock("MWindow::reset_brender");
		brender->allocate_map(0, 0, mainsession->brender_end);
		cwindow->playback_engine->reset_brender();
		cwindow->stop_playback();
		brender_lock->unlock();
	}
}

void MWindow::stop_brender()
{
	if(brender) brender->stop();
}

int MWindow::brender_available(ptstime postime)
{
	int result = 0;

	brender_lock->lock("MWindow::brender_available");
	if(brender)
	{
		if(brender->map_valid)
		{
			brender->map_lock->lock("MWindow::brender_available");
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
	draw_canvas_overlays();
}

void MWindow::load_filenames(ArrayList<char*> *filenames,
	int load_mode)
{
	int result = 0;
	ptstime pos, dur;
	int oloadmode = load_mode;
	ptstime original_length;
	ptstime original_preview_end;

	save_defaults();
	gui->start_hourglass();

// Stop playback EDL-s are going modified
	cwindow->stop_playback();
	vwindow->stop_playback();

	if(load_mode == LOADMODE_REPLACE || load_mode == LOADMODE_REPLACE_CONCATENATE)
	{
		reset_caches();
		assetlist_global.reset_inuse();
		cliplist_global.remove_all_objects();
		vwindow->remove_source();
		vwindow_edl->reset_instance();
		master_edl->reset_instance();
		set_filename(0);
	}

	original_length = master_edl->duration();
	original_preview_end = master_edl->local_session->preview_end;

	for(int i = 0; i < filenames->total; i++)
	{
// Get type of file
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
			show_message("Loading %s", new_asset->path);
		}
// Fill asset data
		result = assetlist_global.check_asset(new_asset);

		switch(result)
		{
// Convert media file to EDL
		case FILE_OK:
			if(new_asset->nb_programs)
			{
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
							master_edl->init_edl();
							load_mode = LOADMODE_NEW_TRACKS;
							if(!master_edl->local_session->clip_title[0])
								master_edl->local_session->set_clip_title(new_asset->path);
							break;
						case LOADMODE_REPLACE_CONCATENATE:
							master_edl->init_edl();
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
				new_asset = assetlist_global.add_asset(new_asset);
				mainindexes->add_next_asset(new_asset);

				if(load_mode != LOADMODE_RESOURCESONLY)
				{
					master_edl->update_assets(new_asset);

					switch(load_mode)
					{
					case LOADMODE_REPLACE:
						master_edl->init_edl();
						load_mode = LOADMODE_NEW_TRACKS;
						if(!master_edl->local_session->clip_title[0])
							master_edl->local_session->set_clip_title(new_asset->path);
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
						master_edl->init_edl();
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
			delete new_asset;
			new_asset = 0;
			break;

// Unknown format
		case FILE_UNRECOGNIZED_CODEC:
		{
// Test index file
			new_asset->nb_streams = 0;
			if(!(result = new_asset->indexfiles[0].open_index(new_asset, 0)))
				new_asset->indexfiles[0].close_index();
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
				int cx, cy;
				struct streamdesc *sdsc = &new_asset->streams[0];

				fs.extract_name(string, new_asset->path);

				strcat(string, _("'s format couldn't be determined."));
				new_asset->format = FILE_PCM;
				sdsc->channels = defaults->get("AUDIO_CHANNELS", 2);
				sdsc->sample_rate = defaults->get("SAMPLE_RATE", 44100);
				new_asset->pcm_format = defaults->get("PCM_FORMAT", "s16le");
				strcpy(sdsc->codec, new_asset->pcm_format);
				sdsc->options = STRDSC_AUDIO;
				new_asset->nb_streams = 1;
				get_abs_cursor_pos(&cx, &cy);
				FileFormat fwindow(new_asset, string, cx, cy);
				result = fwindow.run_window();
				if(!result)
				{
					if(SampleRateSelection::limits(&sdsc->sample_rate) < 0)
						errorbox(_("Sample rate is out of limits (%d..%d).\nCorrection applied."),
							MIN_SAMPLE_RATE, MAX_SAMPLE_RATE);

					defaults->update("AUDIO_CHANNELS", sdsc->channels);
					defaults->update("SAMPLE_RATE", sdsc->sample_rate);
					defaults->update("PCM_FORMAT", new_asset->pcm_format);
					strcpy(sdsc->codec, new_asset->pcm_format);
					new_asset->nb_streams = 1;
					defaults->delete_key("AUDIO_BITS");
					defaults->delete_key("BYTE_ORDER");
					defaults->delete_key("SIGNED_");
					defaults->delete_key("HEADER");
					save_defaults();
				}
			}

// Append to list
			if(!result)
			{
// Recalculate length
				result = assetlist_global.check_asset(new_asset);

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
							pos = master_edl->local_session->get_selectionstart();
							dur = master_edl->tracks->append_asset(new_asset,
								pos);
							master_edl->local_session->set_selection(pos + dur);
							break;
						case LOADMODE_REPLACE_CONCATENATE:
							master_edl->init_edl();
							load_mode = LOADMODE_CONCATENATE;
							break;
						case LOADMODE_REPLACE:
							master_edl->init_edl();
							load_mode = LOADMODE_NEW_TRACKS;
							break;
						}
					}
					new_asset = 0;
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
				EDL *cur_edl;
				FileXML xml_file;
				xml_file.read_from_file(filenames->values[i]);
				result = 0;
				delete new_asset;
				new_asset = 0;

				if(load_mode == LOADMODE_REPLACE ||
					load_mode == LOADMODE_REPLACE_CONCATENATE)
				{
					master_edl->load_xml(&xml_file, edlsession);
					set_filename(filenames->values[i]);
					cur_edl = master_edl;
					new_edl = 0;
					if(load_mode == LOADMODE_REPLACE_CONCATENATE)
						load_mode = LOADMODE_CONCATENATE;
					else
						load_mode = LOADMODE_NEW_TRACKS;
					gui->mainmenu->update_toggles();
					gwindow->gui->update_toggles();
				}
				else
				{
// Load EDL for pasting
					new_edl = new EDL(0);
					new_edl->copy_session(master_edl);
					new_edl->load_xml(&xml_file, 0);
					cur_edl = new_edl;
				}

				for(int i = 0; i < cur_edl->assets->total; i++)
					mainindexes->add_next_asset(cur_edl->assets->values[i]);

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
							0, -1, 0);
					}
					delete new_edl;
					new_edl = 0;
				}
				else if(result)
					set_filename(0);
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

	default_message();

	mainindexes->start_build();
	master_edl->check_master_track();
	assetlist_global.remove_unused();
	// Synchronize edl with assetlist
	for(Asset *g_asset = assetlist_global.first; g_asset; g_asset = g_asset->next)
		master_edl->update_assets(g_asset);
// Fix preview range
	if(EQUIV(original_length, original_preview_end))
		master_edl->local_session->preview_end = master_edl->duration();
	update_project(oloadmode);

	undo->update_undo(_("load"), LOAD_ALL, 0);
	gui->stop_hourglass();
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
	gui->mainmenu->show_vwindow->set_checked(1);
}

void MWindow::mark_vwindow_hidden()
{
	mainsession->show_vwindow = 0;
	gui->mainmenu->show_vwindow->set_checked(0);
}

void MWindow::show_awindow()
{
	mainsession->show_awindow = 1;
	awindow->gui->show_window();
	awindow->gui->raise_window();
	gui->mainmenu->show_awindow->set_checked(1);
}

void MWindow::mark_awindow_hidden()
{
	mainsession->show_awindow = 0;
	gui->mainmenu->show_awindow->set_checked(0);
}

void MWindow::show_cwindow()
{
	mainsession->show_cwindow = 1;
	cwindow->show_window();
	gui->mainmenu->show_cwindow->set_checked(1);
}

void MWindow::mark_cwindow_hidden()
{
	mainsession->show_cwindow = 0;
	gui->mainmenu->show_cwindow->set_checked(0);
}

void MWindow::show_gwindow()
{
	mainsession->show_gwindow = 1;
	gwindow->gui->show_window();
	gwindow->gui->raise_window();
	gui->mainmenu->show_gwindow->set_checked(1);
}

void MWindow::mark_gwindow_hidden()
{
	mainsession->show_gwindow = 0;
	gui->mainmenu->show_gwindow->set_checked(0);
}

void MWindow::show_lwindow()
{
	mainsession->show_lwindow = 1;
	lwindow->gui->show_window();
	lwindow->gui->raise_window();
	gui->mainmenu->show_lwindow->set_checked(1);
}

void MWindow::mark_lwindow_hidden()
{
	mainsession->show_lwindow = 0;
	gui->mainmenu->show_lwindow->set_checked(0);
}

void MWindow::show_ruler()
{
	mainsession->show_ruler = 1;
	ruler->gui->show_window();
	ruler->gui->raise_window();
	gui->mainmenu->show_ruler->set_checked(1);
}

void MWindow::mark_ruler_hidden()
{
	mainsession->show_ruler = 0;
	gui->mainmenu->show_ruler->set_checked(0);
}

void MWindow::toggle_loop_playback()
{
	master_edl->local_session->loop_playback =
		!master_edl->local_session->loop_playback;
	set_loop_boundaries();
	save_backup();

	draw_canvas_overlays();
	sync_parameters(0);
}

void MWindow::set_auto_keyframes(int value)
{
	edlsession->auto_keyframes = value;
	gui->mbuttons->edit_panel->keyframe->update(value);
	cwindow->gui->edit_panel->keyframe->update(value);
	cwindow->gui->update_tool();
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
}

void MWindow::sync_parameters(int brender_restart)
{
	master_edl->reset_plugins();
	if(!cwindow->playback_engine->is_playing_back)
		cwindow->playback_engine->send_command(CURRENT_FRAME);
	if(brender_restart)
		restart_brender();
}

void MWindow::age_caches()
{
	size_t prev_memory_usage;
	size_t memory_usage;

	memory_usage = gui->canvas->get_cache_size();
	do {
		if(memory_usage > preferences->cache_size)
			gui->canvas->cache_delete_oldest();
		prev_memory_usage = memory_usage;
		memory_usage = gui->canvas->get_cache_size();
	} while(prev_memory_usage != memory_usage &&
		memory_usage > preferences->cache_size);
}

void MWindow::change_meter_format(int min_db, int max_db)
{
	gui->patchbay->change_meter_format(
		edlsession->min_meter_db,
		edlsession->max_meter_db);
}

void MWindow::stop_hourglass()
{
	gui->stop_hourglass();
}

void MWindow::start_hourglass()
{
	gui->start_hourglass();
}

void MWindow::update_undo_text(const char *text)
{
	gui->mainmenu->undo->update_caption(text);
}

void MWindow::update_redo_text(const char *text)
{
	gui->mainmenu->redo->update_caption(text);
}

void MWindow::add_aeffect_menu(const char *title)
{
	gui->mainmenu->add_aeffect(title);
}

void MWindow::add_veffect_menu(const char *title)
{
	gui->mainmenu->add_veffect(title);
}

void MWindow::show_plugin(Plugin *plugin)
{
	int new_win = 0;

	plugin_gui_lock->lock("MWindow::show_plugin");
	new_win = plugin->show_plugin_gui();
	plugin_gui_lock->unlock();

	if(new_win && plugin->plugin_server->status_gui)
	{
		ptstime cur_pts = master_edl->local_session->get_selectionstart(1);

		if(cur_pts >= plugin->get_pts() && cur_pts < plugin->end_pts())
		{
			// Give some time for gui opening
			usleep(100000);
			sync_parameters(0);
		}
	}
}

void MWindow::update_plugin_guis()
{
	plugin_gui_lock->lock("MWindow::update_plugin_guis");

	master_edl->update_plugin_guis();

	plugin_gui_lock->unlock();
}

// Reset everything after a load.
void MWindow::update_project(int load_mode)
{
	restart_brender();
	master_edl->tracks->update_y_pixels(theme);

	update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK | WUPD_BUTTONBAR);

	cwindow->update(WUPD_SCROLLBARS | WUPD_TOOLWIN | WUPD_OPERATION |
		WUPD_TIMEBAR | WUPD_OVERLAYS);

	if(load_mode == LOADMODE_REPLACE ||
		load_mode == LOADMODE_REPLACE_CONCATENATE)
	{
		vwindow->change_source();
	}

	cwindow->gui->slider->set_position();
	cwindow->playback_engine->send_command(CURRENT_FRAME);
	awindow->gui->async_update_assets();
}

void MWindow::rebuild_indices(Asset *asset)
{
	asset->remove_indexes();
	mainindexes->add_next_asset(asset);
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
	master_edl->save_xml(&file, BACKUP_PATH, EDL_BACKUP);
	strcpy(path, BACKUP_PATH);
	fs.complete_path(path);

	if(file.write_to_file(path))
		show_message(_("Couldn't open %s for writing."), path);
}

void MWindow::reset_caches()
{
	gui->canvas->reset_caches();
	BC_Resources::tmpframes.delete_unused();
	audio_frames.delete_unused();
}

void MWindow::remove_asset_from_caches(Asset *asset)
{
	cwindow->stop_playback();
	vwindow->stop_playback();

	gui->canvas->remove_asset_from_caches(asset);
	cwindow->playback_engine->release_asset(asset);
	vwindow->playback_engine->release_asset(asset);
}

void MWindow::remove_assets_from_project(int push_undo)
{
	cwindow->stop_playback();
	vwindow->stop_playback();

	for(int i = 0; i < mainsession->drag_assets->total; i++)
	{
		Asset *asset = mainsession->drag_assets->values[i];
		remove_asset_from_caches(asset);
		vwindow->remove_source(asset);
	}
// Remove from VWindow.
	for(int i = 0; i < mainsession->drag_clips->total; i++)
	{
		if(mainsession->drag_clips->values[i]->id == vwindow_edl->id)
			vwindow->remove_source();
	}
	master_edl->remove_from_project(mainsession->drag_assets);
	vwindow_edl->remove_from_project(mainsession->drag_assets);
	cliplist_global.remove_from_project(mainsession->drag_clips);
	assetlist_global.remove_assets(mainsession->drag_assets);
	save_backup();
	if(push_undo) undo->update_undo(_("remove assets"), LOAD_ALL);

	update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_CLOCK);

	awindow->gui->async_update_assets();
	sync_parameters();
}

void MWindow::update_gui(int options, ptstime new_position)
{
	master_edl->tracks->update_y_pixels(theme);

	if(options & WUPD_SCROLLBARS)
		gui->get_scrollbars();
	if(options & WUPD_TIMEDEPS)
		gui->zoombar->redraw_time_dependancies();
	if(options & WUPD_TIMEBAR)
		gui->timebar->update();
	if(options & WUPD_ZOOMBAR)
		gui->zoombar->update();
	if(options & WUPD_PATCHBAY)
		gui->patchbay->update();
	if(options & WUPD_CLOCK)
	{
		if(new_position < 0)
			gui->mainclock->update(
				master_edl->local_session->get_selectionstart(1));
		else
			gui->mainclock->update(new_position);
	}

	if(options & WUPD_CANVAS)
	{
		gui->canvas->draw(options & WUPD_CANVAS);
		gui->canvas->flash();
		gui->canvas->activate();
	}

	if(options & WUPD_BUTTONBAR)
		gui->mbuttons->update();

	if(options & WUPD_TOGGLES)
		gui->mainmenu->update_toggles();

	if(options & WUPD_LABELS)
		gui->timebar->update_labels();

	if(options & WUPD_TOGLIGHTS)
		gui->timebar->update_highlights();

	if(options & WUPD_CURSOR)
		gui->cursor->update();

	if(options & WUPD_ZCLOCKS)
		gui->zoombar->update_clocks();

// Can't age if the cache called this to draw missing picons
	if((options & WUPD_CANVREDRAW) == 0)
		age_caches();
}

int MWindow::stop_composer()
{
	return cwindow->stop_playback();
}

int MWindow::stop_playback()
{
	return vwindow->stop_playback() | cwindow->stop_playback();
}

void MWindow::get_abs_cursor_pos(int *abs_x, int *abs_y)
{
	gui->get_abs_cursor_pos(abs_x, abs_y);
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

// ================================= synchronization

void MWindow::interrupt_indexes()
{
	mainindexes->interrupt_build();
}

void MWindow::draw_indexes(Asset *asset)
{
	gui->canvas->draw_indexes(asset);
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
	char string[BCTEXTLEN];

	update_gui(WUPD_TIMEBAR | WUPD_CLOCK | WUPD_TIMEDEPS);
	show_message(_("Using %s."), Units::print_time_format(edlsession->time_format, string));
}


void MWindow::set_filename(const char *filename)
{
	if(filename)
		strcpy(mainsession->filename, filename);
	else
		mainsession->filename[0] = 0;

	if(gui)
	{
		if(!filename || filename[0] == 0)
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

void MWindow::add_load_path(const char *path)
{
	gui->mainmenu->add_load(path);
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
	if(master_edl->duration())
	{
		start = 0;
		end = master_edl->duration();
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

VFrame *MWindow::get_window_icon()
{
	return theme->get_image("mwindow_icon");
}

void MWindow::draw_canvas_overlays()
{
	gui->cursor->hide();
	gui->canvas->draw_overlays();
	gui->cursor->show();
	gui->canvas->flash();
}

ptstime MWindow::trackcanvas_visible()
{
	return gui->canvas->time_visible();
}

void MWindow::activate_canvas()
{
	gui->canvas->activate();
}

void MWindow::deactivate_canvas()
{
	gui->canvas->deactivate();
}

void MWindow::show_message(const char *fmt, ...)
{
	char bufr[BCTEXTLEN];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(bufr, 1024, fmt, ap);
	va_end(ap);

	gui->statusbar->status_text->set_color(theme->message_normal);
	gui->statusbar->status_text->update(bufr);
}

void MWindow::default_message()
{
	gui->statusbar->status_text->set_color(theme->message_normal);
	gui->statusbar->status_text->update(_("Welcome to " PROGRAM_NAME));
}

void MWindow::show_program_status()
{
	size_t mc;
	int count, inuse;

	printf("%s status:\n", version_name);
	if(master_edl)
	{
		printf(" Audio channels %d tracks %d samplerate %d\n",
			edlsession->audio_channels, edlsession->audio_tracks,
			edlsession->sample_rate);
		printf( " Video [%d,%d] tracks %d framerate %.2f '%s'\n",
			edlsession->output_w,
			edlsession->output_h,
			edlsession->video_tracks,
			edlsession->frame_rate,
			ColorModels::name(edlsession->color_model));
	}
	else
		printf(" No edl\n");

	printf(" Internal encoding: '%s'\n", BC_Resources::encoding);
	gui->canvas->show_cache_status(1);
	mc = BC_Resources::tmpframes.get_size(&count, &inuse);
	printf(" Video tmpframes %d/%d %zuk\n", inuse, count, mc);
	mc = audio_frames.get_size(&count, &inuse);
	printf(" Audio tmpframes %d/%d %zuk\n", inuse, count, mc);
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
	BC_Resources::tmpframes.dump(4);
}

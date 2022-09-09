// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aboutprefs.h"
#include "asset.h"
#include "audiodevice.inc"
#include "bclistboxitem.h"
#include "bcresources.h"
#include "preferencesthread.h"
#include "bcsignals.h"
#include "cache.h"
#include "cinelerra.h"
#include "cplayback.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "filesystem.h"
#include "fonts.h"
#include "interfaceprefs.h"
#include "keys.h"
#include "language.h"
#include "levelwindow.h"
#include "levelwindowgui.h"
#include "mainerror.h"
#include "meterpanel.h"
#include "miscprefs.h"
#include "mutex.h"
#include "mwindow.h"
#include "performanceprefs.h"
#include "playbackconfig.h"
#include "playbackengine.h"
#include "playbackprefs.h"
#include "preferences.h"
#include "question.h"
#include "selection.h"
#include "theme.h"
#include "videodevice.inc"
#include "vwindow.h"
#include "vwindowgui.h"

#include <string.h>

#define WIDTH 810
#define HEIGHT 690


PreferencesMenuitem::PreferencesMenuitem()
 : BC_MenuItem(_("Preferences..."), "Shift+P", 'P')
{
	set_shift(1);
	thread = new PreferencesThread();
}

PreferencesMenuitem::~PreferencesMenuitem()
{
	delete thread;
}


int PreferencesMenuitem::handle_event() 
{
	if(!thread->running())
	{
		thread->start();
	}
	else
	{
// window_lock has to be locked but window can't be locked until after
// it is known to exist, so we neglect window_lock for now
		if(thread->window)
		{
			thread->window_lock->lock("SetFormat::handle_event");
			thread->window->raise_window();
			thread->window_lock->unlock();
		}
	}
	return 1;
}


PreferencesThread::PreferencesThread()
 : Thread()
{
	window = 0;
	thread_running = 0;
	window_lock = new Mutex("PreferencesThread::window_lock");
}

PreferencesThread::~PreferencesThread()
{
	delete window_lock;
}

void PreferencesThread::run()
{
	int need_new_indexes;
	int x, y;

	preferences = new Preferences;
	edl = new EDL(0);
	this_edlsession = new EDLSession();
	current_dialog = mwindow_global->defaults->get("DEFAULTPREF", 0);
	preferences->copy_from(preferences_global);
	this_edlsession->copy(edlsession);
	edl->copy_session(master_edl, this_edlsession);
	redraw_indexes = 0;
	redraw_meters = 0;
	redraw_times = 0;
	redraw_overlays = 0;
	close_assets = 0;
	reload_plugins = 0;
	need_new_indexes = 0;
	rerender = 0;

	BC_Resources::get_root_size(&x, &y);
	x = x / 2 - WIDTH / 2;
	y = y / 2 - HEIGHT / 2;

	window_lock->lock("PreferencesThread::run 1");
	window = new PreferencesWindow(this, x, y);
	window_lock->unlock();

	thread_running = 1;
	int result = window->run_window();

	thread_running = 0;
	if(!result)
	{
		if(!preferences->brender_asset->equivalent(*preferences_global->brender_asset,
				STRDSC_VIDEO))
		{
			if(preferences->brender_asset->renderprofile_path[0])
				preferences->brender_asset->save_render_profile();
			mwindow_global->reset_brender();
		}
		apply_settings();
		mwindow_global->save_defaults();
		mwindow_global->restart_brender();
	}

	window_lock->lock("PreferencesThread::run 2");
	delete window;
	window = 0;
	window_lock->unlock();
	delete preferences;
	delete edl;
	delete this_edlsession;

	mwindow_global->defaults->update("DEFAULTPREF", current_dialog);
}

void PreferencesThread::update_framerate()
{
	if(thread_running && window)
	{
		window->update_framerate();
	}
}

void PreferencesThread::update_playstatistics()
{
	if(thread_running && window)
		window->update_playstatistics();
}

void PreferencesThread::apply_settings()
{
// Compare sessions

	AudioOutConfig *this_aconfig = this_edlsession->playback_config->aconfig;
	VideoOutConfig *this_vconfig = this_edlsession->playback_config->vconfig;
	AudioOutConfig *aconfig = edlsession->playback_config->aconfig;
	VideoOutConfig *vconfig = edlsession->playback_config->vconfig;

	rerender = 
		this_edlsession->need_rerender(edlsession) ||
		(preferences->max_threads != preferences_global->max_threads) ||
		(*this_aconfig != *aconfig) ||
		(*this_vconfig != *vconfig) ||
		!preferences->brender_asset->equivalent(*preferences_global->brender_asset,
			STRDSC_VIDEO);

	if(preferences->use_brender != preferences_global->use_brender)
	{
		redraw_overlays = 1;
		redraw_times = 1;
	}

	// Check index directory
	if(strcmp(preferences_global->index_directory, preferences->index_directory))
	{
		char new_dir[BCTEXTLEN];
		FileSystem fs;
		struct stat stb;
		int nocreate = 0;

		strcpy(new_dir, preferences->index_directory);
		fs.complete_path(new_dir);
		if(!stat(new_dir, &stb))
		{
			if(!S_ISDIR(stb.st_mode))
			{
				errormsg(_("'%s' is not a directory"),
					preferences->index_directory);
				nocreate = 1;
			}
		}
		else
		{
			int cx, cy;

			mwindow_global->get_abs_cursor_pos(&cx, &cy);
			QuestionWindow confirm(0, cx, cy,
				_("Index directory is missing.\nCreate the directory?"));
			if(confirm.run_window())
			{
				if(mkdir(new_dir, S_IREAD | S_IWRITE | S_IEXEC))
				{
					errormsg(_("Can't create directory '%s': %m"),
						preferences->index_directory);
					nocreate = 1;
				}
			}
			else
				nocreate = 1;
		}
		if(nocreate)
			strcpy(preferences->index_directory,
				preferences_global->index_directory);
	}
	edlsession->copy(this_edlsession);
	preferences_global->copy_from(preferences);
	mwindow_global->init_brender();

	if(((edlsession->output_w % 4) ||
		(edlsession->output_h % 4)) &&
		edlsession->playback_config->vconfig->driver == PLAYBACK_X11_GL)
	{
		errormsg(_("This project's dimensions are not multiples of 4 so\n"
			"it can't be rendered by OpenGL."));
	}

	if(redraw_meters)
	{
		mwindow_global->cwindow->gui->meters->change_format(
			edlsession->min_meter_db,
			edlsession->max_meter_db);

		mwindow_global->vwindow->gui->meters->change_format(
			edlsession->min_meter_db,
			edlsession->max_meter_db);

		mwindow_global->change_meter_format(
			edlsession->min_meter_db,
			edlsession->max_meter_db);

		mwindow_global->lwindow->gui->panel->change_format(
			edlsession->min_meter_db,
			edlsession->max_meter_db);
	}

	if(redraw_overlays)
	{
		mwindow_global->draw_canvas_overlays();
		redraw_overlays = 0;
	}

	if(redraw_times)
	{
		mwindow_global->update_gui(WUPD_TIMEBAR | WUPD_CLOCK | WUPD_TIMEDEPS);
		redraw_times = 0;
	}

	if(rerender)
	{
		mwindow_global->cwindow->playback_engine->send_command(CURRENT_FRAME);
		mwindow_global->vwindow->change_source();
	}
}

const char* PreferencesThread::category_to_text(int category)
{
	switch(category)
	{
	case PLAYBACK:
		return _("Playback");
	case PERFORMANCE:
		return _("Performance");
	case INTERFACE:
		return _("Interface");
	case MISC:
		return _("Settings");
	case ABOUT:
		return _("About");
	}
	return "";
}

int PreferencesThread::text_to_category(char *category)
{
	int min_result = -1, result, result_num = 0;
	for(int i = 0; i < CATEGORIES; i++)
	{
		result = labs(strcmp(category_to_text(i), category));
		if(result < min_result || min_result < 0) 
		{
			min_result = result;
			result_num = i;
		}
	}
	return result_num;
}


PreferencesWindow::PreferencesWindow(PreferencesThread *thread,
	int x,
	int y)
 : BC_Window(MWindow::create_title(N_("Preferences")),
	x,
	y,
	WIDTH, 
	HEIGHT,
	BC_INFINITY,
	BC_INFINITY,
	0,
	0,
	1)
{
	BC_Button *button;

	this->thread = thread;
	dialog = 0;
	category = 0;

	set_icon(mwindow_global->get_window_icon());
	theme_global->draw_preferences_bg(this);
	flash();

	x = theme_global->preferencescategory_x;
	y = theme_global->preferencescategory_y;
	for(int i = 0; i < CATEGORIES; i++)
	{
		add_subwindow(category_button[i] = new PreferencesButton(
			thread,
			x,
			y,
			i,
			thread->category_to_text(i),
			(i == thread->current_dialog) ?
				theme_global->get_image_set("category_button_checked") :
				theme_global->get_image_set("category_button")));
		x += category_button[i]->get_w() -
			theme_global->preferences_category_overlap;
	}

	add_subwindow(button = new PreferencesOK(this));
	add_subwindow(new PreferencesApply(thread, this));
	add_subwindow(new PreferencesCancel(this));

	set_current_dialog(thread->current_dialog);
	show_window();
}

PreferencesWindow::~PreferencesWindow()
{
	delete category;
	if(dialog) delete dialog;
	for(int i = 0; i < categories.total; i++)
		delete categories.values[i];
}

void PreferencesWindow::update_framerate()
{
	if(thread->current_dialog == 0)
	{
		thread->this_edlsession->actual_frame_rate =
			edlsession->actual_frame_rate;
		dialog->draw_framerate();
		flash();
	}
}

void PreferencesWindow::update_playstatistics()
{
	if(thread->current_dialog == 0)
	{
		thread->this_edlsession->frame_count =
			edlsession->frame_count;
		thread->this_edlsession->frames_late =
			edlsession->frames_late;
		thread->this_edlsession->avg_delay =
			edlsession->avg_delay;
		dialog->draw_playstatistics();
		flash();
	}
}

void PreferencesWindow::set_current_dialog(int number)
{
	thread->current_dialog = number;
	if(dialog) delete dialog;
	dialog = 0;

// Redraw category buttons
	for(int i = 0; i < CATEGORIES; i++)
	{
		if(i == number)
		{
			category_button[i]->set_images(
				theme_global->get_image_set("category_button_checked"));
		}
		else
		{
			category_button[i]->set_images(
				theme_global->get_image_set("category_button"));
		}
		category_button[i]->draw_face();

// Copy face to background for next button's overlap.
// Still can't to state changes right.
	}

	switch(number)
	{
	case PreferencesThread::PLAYBACK:
		add_subwindow(dialog = new PlaybackPrefs(this));
		break;

	case PreferencesThread::PERFORMANCE:
		add_subwindow(dialog = new PerformancePrefs(this));
		break;

	case PreferencesThread::INTERFACE:
		add_subwindow(dialog = new InterfacePrefs(this));
		break;

	case PreferencesThread::MISC:
		add_subwindow(dialog = new MiscPrefs(this));
		break;

	case PreferencesThread::ABOUT:
		add_subwindow(dialog = new AboutPrefs(this));
		break;
	}

	if(dialog)
	{
		dialog->draw_top_background(this, 0, 0, dialog->get_w(), dialog->get_h());
		dialog->flash();
		dialog->show();
	}
}


PreferencesButton::PreferencesButton(PreferencesThread *thread,
	int x, 
	int y,
	int category,
	const char *text,
	VFrame **images)
 : BC_GenericButton(x, y, text, images)
{
	this->thread = thread;
	this->category = category;
}

int PreferencesButton::handle_event()
{
	thread->window->set_current_dialog(category);
	return 1;
}


PreferencesDialog::PreferencesDialog(PreferencesWindow *pwindow)
 : BC_SubWindow(10, 40,
	pwindow->get_w() - 20,
	pwindow->get_h() - BC_GenericButton::calculate_h() - 10 - 40)
{
	this->pwindow = pwindow;
	preferences = pwindow->thread->preferences;
}

// ============================== category window

PreferencesApply::PreferencesApply(PreferencesThread *thread,
	PreferencesWindow *window)
 : BC_GenericButton(window->get_w() / 2 - BC_GenericButton::calculate_w(window, _("Apply")) / 2,
	window->get_h() - BC_GenericButton::calculate_h() - 10, 
	_("Apply"))
{
	this->thread = thread;
}

int PreferencesApply::handle_event()
{
	thread->apply_settings();
	return 1;
}


PreferencesOK::PreferencesOK(PreferencesWindow *window)
 : BC_GenericButton(10, 
	window->get_h() - BC_GenericButton::calculate_h() - 10,
	_("OK"))
{
	this->window = window;
}

int PreferencesOK::keypress_event()
{
	if(get_keypress() == RETURN)
	{
		window->set_done(0);
		return 1;
	}
	return 0;
}

int PreferencesOK::handle_event()
{
	window->set_done(0);
	return 1;
}


PreferencesCancel::PreferencesCancel(PreferencesWindow *window)
 : BC_GenericButton(window->get_w() - BC_GenericButton::calculate_w(window, _("Cancel")) - 10,
	window->get_h() - BC_GenericButton::calculate_h() - 10,
	_("Cancel"))
{
	this->window = window;
}

int PreferencesCancel::keypress_event()
{
	if(get_keypress() == ESC)
	{
		window->set_done(1);
		return 1;
	}
	return 0;
}

int PreferencesCancel::handle_event()
{
	window->set_done(1);
	return 1;
}


PreferencesCategory::PreferencesCategory(PreferencesThread *thread, int x, int y)
 : BC_PopupTextBox(thread->window, 
		&thread->window->categories,
		thread->category_to_text(thread->current_dialog),
		x, 
		y, 
		200,
		150)
{
	this->thread = thread;
}

int PreferencesCategory::handle_event()
{
	thread->window->set_current_dialog(thread->text_to_category(get_text()));
	return 1;
}

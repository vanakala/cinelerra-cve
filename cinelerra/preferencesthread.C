
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

#include "aboutprefs.h"
#include "asset.h"
#include "audiodevice.inc"
#include "bcsignals.h"
#include "cache.h"
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
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "performanceprefs.h"
#include "playbackengine.h"
#include "playbackprefs.h"
#include "preferences.h"
#include "recordprefs.h"
#include "theme.h"
#include "trackcanvas.h"
#include "transportque.h"
#include "vwindow.h"
#include "vwindowgui.h"

#include <string.h>



#define WIDTH 750
#define HEIGHT 780


PreferencesMenuitem::PreferencesMenuitem(MWindow *mwindow)
 : BC_MenuItem(_("Preferences..."), "Shift+P", 'P')
{
	this->mwindow = mwindow; 

	set_shift(1);
	thread = new PreferencesThread(mwindow);
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
			thread->window->lock_window("PreferencesMenuitem::handle_event");
			thread->window->raise_window();
			thread->window->unlock_window();
			thread->window_lock->unlock();
		}
	}
	return 1;
}




PreferencesThread::PreferencesThread(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
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

	preferences = new Preferences;
	edl = new EDL;
	edl->create_objects();
	current_dialog = mwindow->defaults->get("DEFAULTPREF", 0);
	preferences->copy_from(mwindow->preferences);
	edl->copy_session(mwindow->edl);
	redraw_indexes = 0;
	redraw_meters = 0;
	redraw_times = 0;
	redraw_overlays = 0;
	close_assets = 0;
	reload_plugins = 0;
	need_new_indexes = 0;
	rerender = 0;

 	int x = mwindow->gui->get_root_w(0, 1) / 2 - WIDTH / 2;
	int y = mwindow->gui->get_root_h(1) / 2 - HEIGHT / 2;

	window_lock->lock("PreferencesThread::run 1");
	window = new PreferencesWindow(mwindow, this, x, y);
	window->create_objects();
	window_lock->unlock();

	thread_running = 1;
	int result = window->run_window();

	thread_running = 0;
	if(!result)
	{
		apply_settings();
		mwindow->save_defaults();
	}

	window_lock->lock("PreferencesThread::run 2");
	delete window;
	window = 0;
	window_lock->unlock();
	delete preferences;
	delete edl;

	mwindow->defaults->update("DEFAULTPREF", current_dialog);
}

int PreferencesThread::update_framerate()
{
	if(thread_running && window)
	{
		window->update_framerate();
	}
	return 0;
}

int PreferencesThread::apply_settings()
{
// Compare sessions 											


	AudioOutConfig *this_aconfig = edl->session->playback_config->aconfig;
	VideoOutConfig *this_vconfig = edl->session->playback_config->vconfig;
	AudioOutConfig *aconfig = mwindow->edl->session->playback_config->aconfig;
	VideoOutConfig *vconfig = mwindow->edl->session->playback_config->vconfig;

	
	rerender = 
		edl->session->need_rerender(mwindow->edl->session) ||
		(preferences->force_uniprocessor != preferences->force_uniprocessor) ||
		(*this_aconfig != *aconfig) ||
		(*this_vconfig != *vconfig) ||
		!preferences->brender_asset->equivalent(*mwindow->preferences->brender_asset, 0, 1);




	mwindow->edl->copy_session(edl, 1);
	mwindow->preferences->copy_from(preferences);
	mwindow->init_brender();

	if(((mwindow->edl->session->output_w % 4) || 
		(mwindow->edl->session->output_h % 4)) && 
		mwindow->edl->session->playback_config->vconfig->driver == PLAYBACK_X11_GL)
	{
		MainError::show_error(
			_("This project's dimensions are not multiples of 4 so\n"
			"it can't be rendered by OpenGL."));
	}


	if(redraw_meters)
	{
		mwindow->cwindow->gui->lock_window("PreferencesThread::apply_settings");
		mwindow->cwindow->gui->meters->change_format(edl->session->meter_format,
			edl->session->min_meter_db,
			edl->session->max_meter_db);
		mwindow->cwindow->gui->unlock_window();



		mwindow->vwindow->gui->lock_window("PreferencesThread::apply_settings");
		mwindow->vwindow->gui->meters->change_format(edl->session->meter_format,
			edl->session->min_meter_db,
			edl->session->max_meter_db);
		mwindow->vwindow->gui->unlock_window();



		mwindow->gui->lock_window("PreferencesThread::apply_settings 1");
		mwindow->gui->patchbay->change_meter_format(edl->session->meter_format,
			edl->session->min_meter_db,
			edl->session->max_meter_db);
		mwindow->gui->unlock_window();



		mwindow->lwindow->gui->lock_window("PreferencesThread::apply_settings");
		mwindow->lwindow->gui->panel->change_format(edl->session->meter_format,
			edl->session->min_meter_db,
			edl->session->max_meter_db);
		mwindow->lwindow->gui->unlock_window();
	}

	if(redraw_overlays)
	{
		mwindow->gui->lock_window("PreferencesThread::apply_settings 2");
		mwindow->gui->canvas->draw_overlays();
		mwindow->gui->canvas->flash();
		mwindow->gui->unlock_window();
	}

	if(redraw_times)
	{
		mwindow->gui->lock_window("PreferencesThread::apply_settings 3");
		mwindow->gui->update(0, 0, 1, 0, 0, 1, 0);
		mwindow->gui->redraw_time_dependancies();
		mwindow->gui->unlock_window();
	}

	if(rerender)
	{
//printf("PreferencesThread::apply_settings 1\n");
// This doesn't stop and restart, only reloads the assets before
// the next play command.
		mwindow->cwindow->playback_engine->que->send_command(CURRENT_FRAME,
			CHANGE_ALL,
			mwindow->edl,
			1);
//printf("PreferencesThread::apply_settings 10\n");
	}

	if(redraw_times || redraw_overlays)
	{
		mwindow->gui->lock_window("PreferencesThread::apply_settings 4");
		mwindow->gui->flush();
		mwindow->gui->unlock_window();
	}
	return 0;
}

char* PreferencesThread::category_to_text(int category)
{
	switch(category)
	{
		case PLAYBACK:
			return _("Playback");
			break;
		case RECORD:
			return _("Recording");
			break;
		case PERFORMANCE:
			return _("Performance");
			break;
		case INTERFACE:
			return _("Interface");
			break;
		case ABOUT:
			return _("About");
			break;
	}
	return "";
}

int PreferencesThread::text_to_category(char *category)
{
SET_TRACE
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
SET_TRACE
	return result_num;
}








PreferencesWindow::PreferencesWindow(MWindow *mwindow, 
	PreferencesThread *thread,
	int x,
	int y)
 : BC_Window(PROGRAM_NAME ": Preferences", 
 	x,
	y,
 	WIDTH, 
	HEIGHT,
	(int)BC_INFINITY,
	(int)BC_INFINITY,
	0,
	0,
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
	dialog = 0;
	category = 0;
}

PreferencesWindow::~PreferencesWindow()
{
	delete category;
	if(dialog) delete dialog;
	for(int i = 0; i < categories.total; i++)
		delete categories.values[i];
}

int PreferencesWindow::create_objects()
{
	BC_Button *button;



	mwindow->theme->draw_preferences_bg(this);
	flash();

	int x = mwindow->theme->preferencescategory_x;
	int y = mwindow->theme->preferencescategory_y;
	for(int i = 0; i < CATEGORIES; i++)
	{
		add_subwindow(category_button[i] = new PreferencesButton(mwindow,
			thread,
			x,
			y,
			i,
			thread->category_to_text(i),
			(i == thread->current_dialog) ?
				mwindow->theme->get_image_set("category_button_checked") : 
				mwindow->theme->get_image_set("category_button")));
		x += category_button[i]->get_w() -
			mwindow->theme->preferences_category_overlap;
	}


// 	for(int i = 0; i < CATEGORIES; i++)
// 		categories.append(new BC_ListBoxItem(thread->category_to_text(i)));
// 	category = new PreferencesCategory(mwindow, 
// 		thread, 
// 		mwindow->theme->preferencescategory_x, 
// 		mwindow->theme->preferencescategory_y);
// 	category->create_objects();


	add_subwindow(button = new PreferencesOK(mwindow, thread));
	add_subwindow(new PreferencesApply(mwindow, thread));
	add_subwindow(new PreferencesCancel(mwindow, thread));

	set_current_dialog(thread->current_dialog);
	show_window();
	return 0;
}

int PreferencesWindow::update_framerate()
{
	lock_window("PreferencesWindow::update_framerate");
	if(thread->current_dialog == 0)
	{
		thread->edl->session->actual_frame_rate = 
			mwindow->edl->session->actual_frame_rate;
		dialog->draw_framerate();
		flash();
	}
	unlock_window();
	return 0;
}

int PreferencesWindow::set_current_dialog(int number)
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
				mwindow->theme->get_image_set("category_button_checked"));
		}
		else
		{
			category_button[i]->set_images(
				mwindow->theme->get_image_set("category_button"));
		}
		category_button[i]->draw_face();

// Copy face to background for next button's overlap.
// Still can't to state changes right.
	}

	switch(number)
	{
		case PreferencesThread::PLAYBACK:
			add_subwindow(dialog = new PlaybackPrefs(mwindow, this));
			break;
	
		case PreferencesThread::RECORD:
			add_subwindow(dialog = new RecordPrefs(mwindow, this));
			break;
	
		case PreferencesThread::PERFORMANCE:
			add_subwindow(dialog = new PerformancePrefs(mwindow, this));
			break;
	
		case PreferencesThread::INTERFACE:
			add_subwindow(dialog = new InterfacePrefs(mwindow, this));
			break;
	
		case PreferencesThread::ABOUT:
			add_subwindow(dialog = new AboutPrefs(mwindow, this));
			break;
	}

	if(dialog)
	{
		dialog->draw_top_background(this, 0, 0, dialog->get_w(), dialog->get_h());
		dialog->flash();
		dialog->create_objects();
	}
	return 0;
}











PreferencesButton::PreferencesButton(MWindow *mwindow, 
	PreferencesThread *thread, 
	int x, 
	int y,
	int category,
	char *text,
	VFrame **images)
 : BC_GenericButton(x, y, text, images)
{
	this->mwindow = mwindow;
	this->thread = thread;
	this->category = category;
}

int PreferencesButton::handle_event()
{
	thread->window->set_current_dialog(category);
	return 1;
}









PreferencesDialog::PreferencesDialog(MWindow *mwindow, PreferencesWindow *pwindow)
 : BC_SubWindow(10, 
 	40, 
	pwindow->get_w() - 20, 
 	pwindow->get_h() - BC_GenericButton::calculate_h() - 10 - 40)
{
	this->pwindow = pwindow;
	this->mwindow = mwindow;
	preferences = pwindow->thread->preferences;
}

PreferencesDialog::~PreferencesDialog()
{
}

// ============================== category window




PreferencesApply::PreferencesApply(MWindow *mwindow, PreferencesThread *thread)
 : BC_GenericButton(thread->window->get_w() / 2 - BC_GenericButton::calculate_w(thread->window, _("Apply")) / 2, 
 	thread->window->get_h() - BC_GenericButton::calculate_h() - 10, 
	_("Apply"))
{
	this->mwindow = mwindow;
	this->thread = thread;
}

int PreferencesApply::handle_event()
{
	thread->apply_settings();
	return 1;
}




PreferencesOK::PreferencesOK(MWindow *mwindow, PreferencesThread *thread)
 : BC_GenericButton(10, 
 	thread->window->get_h() - BC_GenericButton::calculate_h() - 10,
	_("OK"))
{
	this->mwindow = mwindow;
	this->thread = thread;
}

int PreferencesOK::keypress_event()
{
	if(get_keypress() == RETURN)
	{
		thread->window->set_done(0);
		return 1;
	}
	return 0;
}
int PreferencesOK::handle_event()
{
	thread->window->set_done(0);
	return 1;
}



PreferencesCancel::PreferencesCancel(MWindow *mwindow, PreferencesThread *thread)
 : BC_GenericButton(thread->window->get_w() - BC_GenericButton::calculate_w(thread->window, _("Cancel")) - 10,
 	thread->window->get_h() - BC_GenericButton::calculate_h() - 10,
 	_("Cancel"))
{
	this->mwindow = mwindow;
	this->thread = thread;
}
int PreferencesCancel::keypress_event()
{
	if(get_keypress() == ESC)
	{
		thread->window->set_done(1);
		return 1;
	}
	return 0;
}

int PreferencesCancel::handle_event()
{
	thread->window->set_done(1);
	return 1;
}










PreferencesCategory::PreferencesCategory(MWindow *mwindow, PreferencesThread *thread, int x, int y)
 : BC_PopupTextBox(thread->window, 
		&thread->window->categories,
		thread->category_to_text(thread->current_dialog),
		x, 
		y, 
		200,
		150)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

PreferencesCategory::~PreferencesCategory()
{
}

int PreferencesCategory::handle_event()
{
SET_TRACE
	thread->window->set_current_dialog(thread->text_to_category(get_text()));
SET_TRACE
	return 1;
}

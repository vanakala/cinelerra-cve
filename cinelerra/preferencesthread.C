#include "aboutprefs.h"
#include "assets.h"
#include "audiodevice.inc"
#include "cache.h"
#include "cplayback.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "defaults.h"
#include "edl.h"
#include "edlsession.h"
#include "filesystem.h"
#include "fonts.h"
#include "interfaceprefs.h"
#include "keys.h"
#include "levelwindow.h"
#include "levelwindowgui.h"
#include "meterpanel.h"
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

PreferencesMenuitem::PreferencesMenuitem(MWindow *mwindow)
 : BC_MenuItem("Preferences...", "Shift+P", 'P')
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
	if(thread->thread_running) return 1;
	thread->start();
	return 1;
}




PreferencesThread::PreferencesThread(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
	window = 0;
	thread_running = 0;
}

PreferencesThread::~PreferencesThread()
{
}

void PreferencesThread::run()
{
	int need_new_indexes;

	preferences = new Preferences;
	edl = new EDL;
	edl->create_objects();
	current_dialog = mwindow->defaults->get("DEFAULTPREF", 0);
	*preferences = *mwindow->preferences;
	edl->copy_session(mwindow->edl);
	redraw_indexes = 0;
	redraw_meters = 0;
	redraw_times = 0;
	redraw_overlays = 0;
	close_assets = 0;
	reload_plugins = 0;
	need_new_indexes = 0;
	rerender = 0;

	window = new PreferencesWindow(mwindow, this);
	window->create_objects();
	thread_running = 1;
	int result = window->run_window();

	thread_running = 0;
	if(!result)
	{
		apply_settings();
		mwindow->save_defaults();
	}

	delete window;
	window = 0;
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


	AudioOutConfig *this_aconfig = edl->session->playback_config[edl->session->playback_strategy].values[0]->aconfig;
	VideoOutConfig *this_vconfig = edl->session->playback_config[edl->session->playback_strategy].values[0]->vconfig;
	AudioOutConfig *aconfig = mwindow->edl->session->playback_config[edl->session->playback_strategy].values[0]->aconfig;
	VideoOutConfig *vconfig = mwindow->edl->session->playback_config[edl->session->playback_strategy].values[0]->vconfig;


	rerender = 
		(edl->session->playback_preload != mwindow->edl->session->playback_preload) ||
		(edl->session->interpolation_type != mwindow->edl->session->interpolation_type) ||
		(edl->session->video_every_frame != mwindow->edl->session->video_every_frame) ||
		(edl->session->real_time_playback != mwindow->edl->session->real_time_playback) ||
		(edl->session->playback_software_position != mwindow->edl->session->playback_software_position) ||
		(edl->session->test_playback_edits != mwindow->edl->session->test_playback_edits) ||
		(edl->session->playback_buffer != mwindow->edl->session->playback_buffer) ||
		(edl->session->audio_module_fragment != mwindow->edl->session->audio_module_fragment) ||
		(edl->session->audio_read_length != mwindow->edl->session->audio_read_length) ||
		(edl->session->playback_strategy != mwindow->edl->session->playback_strategy) ||
		(edl->session->force_uniprocessor != mwindow->edl->session->force_uniprocessor) ||
		(*this_aconfig != *aconfig) ||
		(*this_vconfig != *vconfig) ||
		!preferences->brender_asset->equivalent(*mwindow->preferences->brender_asset, 0, 1);




// TODO: Need to copy just the parameters in PreferencesThread
	mwindow->edl->copy_session(edl);
	*mwindow->preferences = *preferences;
	mwindow->init_brender();

	if(redraw_meters)
	{
		mwindow->cwindow->gui->lock_window();
		mwindow->cwindow->gui->meters->change_format(edl->session->meter_format,
			edl->session->min_meter_db);
		mwindow->cwindow->gui->unlock_window();



		mwindow->vwindow->gui->lock_window();
		mwindow->vwindow->gui->meters->change_format(edl->session->meter_format,
			edl->session->min_meter_db);
		mwindow->vwindow->gui->unlock_window();



		mwindow->gui->lock_window();
		mwindow->gui->patchbay->change_meter_format(edl->session->meter_format,
			edl->session->min_meter_db);
		mwindow->gui->unlock_window();



		mwindow->lwindow->gui->lock_window();
		mwindow->lwindow->gui->panel->change_format(edl->session->meter_format,
			edl->session->min_meter_db);
		mwindow->lwindow->gui->unlock_window();
	}

	if(redraw_overlays)
	{
		mwindow->gui->lock_window();
		mwindow->gui->canvas->draw_overlays();
		mwindow->gui->canvas->flash();
		mwindow->gui->unlock_window();
	}

	if(redraw_times)
	{
		mwindow->gui->lock_window();
		mwindow->gui->update(0, 0, 1, 0, 0, 1, 0);
		mwindow->gui->redraw_time_dependancies();
		mwindow->gui->unlock_window();
	}

	if(rerender)
	{
		mwindow->cwindow->playback_engine->que->send_command(CURRENT_FRAME,
			CHANGE_ALL,
			mwindow->edl,
			1);
	}

	if(redraw_times || redraw_overlays)
	{
		mwindow->gui->flush();
	}
	return 0;
}

char* PreferencesThread::category_to_text(int category)
{
	switch(category)
	{
		case 0:
			return "Playback";
			break;
		case 1:
			return "Recording";
			break;
		case 2:
			return "Performance";
			break;
		case 3:
			return "Interface";
			break;
// 		case 4:
// 			return "Plugin Set";
// 			break;
		case 4:
			return "About";
			break;
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










#define WIDTH 750
#define HEIGHT 700

PreferencesWindow::PreferencesWindow(MWindow *mwindow, 
	PreferencesThread *thread)
 : BC_Window(PROGRAM_NAME ": Preferences", 
 	mwindow->gui->get_root_w() / 2 - WIDTH / 2,
	mwindow->gui->get_root_h() / 2 - HEIGHT / 2,
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
	int x = 10, y = 10;
	BC_Button *button;



	mwindow->theme->draw_preferences_bg(this);
	flash();


//printf("PreferencesWindow::create_objects 1\n");
	for(int i = 0; i < CATEGORIES; i++)
		categories.append(new BC_ListBoxItem(thread->category_to_text(i)));
//	add_subwindow(new BC_Title(x, y, "Category:", LARGEFONT_3D, RED));
	category = new PreferencesCategory(mwindow, thread, x, y);
	category->create_objects();

//printf("PreferencesWindow::create_objects 1\n");
	y += category->get_h();

//printf("PreferencesWindow::create_objects 1\n");
	add_subwindow(button = new BC_OKButton(this));
//printf("PreferencesWindow::create_objects 1\n");
	add_subwindow(new PreferencesApply(mwindow, 
		thread, 
		get_w() / 2 - 50, 
		button->get_y()));
	x = get_w() - 100;
//printf("PreferencesWindow::create_objects 1\n");
	add_subwindow(new BC_CancelButton(this));

//printf("PreferencesWindow::create_objects 1\n");
	set_current_dialog(thread->current_dialog);
//printf("PreferencesWindow::create_objects 1\n");
	show_window();
//printf("PreferencesWindow::create_objects 2\n");
	return 0;
}

int PreferencesWindow::update_framerate()
{
	lock_window();
//printf("PreferencesWindow::update_framerate 1\n");
	if(thread->current_dialog == 0)
	{
//printf("PreferencesWindow::update_framerate 2\n");
		thread->edl->session->actual_frame_rate = 
			mwindow->edl->session->actual_frame_rate;
//printf("PreferencesWindow::update_framerate 3\n");
		dialog->draw_framerate();
//printf("PreferencesWindow::update_framerate 4\n");
		flash();
	}
//printf("PreferencesWindow::update_framerate 5\n");
	unlock_window();
	return 0;
}

int PreferencesWindow::set_current_dialog(int number)
{
	thread->current_dialog = number;
	if(dialog) delete dialog;
	dialog = 0;

	switch(number)
	{
		case 0:
			add_subwindow(dialog = new PlaybackPrefs(mwindow, this));
			break;
	
		case 1:
			add_subwindow(dialog = new RecordPrefs(mwindow, this));
			break;
	
		case 2:
			add_subwindow(dialog = new PerformancePrefs(mwindow, this));
			break;
	
		case 3:
			add_subwindow(dialog = new InterfacePrefs(mwindow, this));
			break;
	
// 		case 4:
// 			add_subwindow(dialog = new PluginPrefs(mwindow, this));
// 			break;
	
		case 4:
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

// ================================== save values



PreferencesDialog::PreferencesDialog(MWindow *mwindow, PreferencesWindow *pwindow)
 : BC_SubWindow(10, 40, pwindow->get_w() - 20, pwindow->get_h() - 100)
{
	this->pwindow = pwindow;
	this->mwindow = mwindow;
	preferences = pwindow->thread->preferences;
}

PreferencesDialog::~PreferencesDialog()
{
}

// ============================== category window




PreferencesApply::PreferencesApply(MWindow *mwindow, PreferencesThread *thread, int x, int y)
 : BC_GenericButton(x, y, "Apply")
{
	this->mwindow = mwindow;
	this->thread = thread;
}

PreferencesApply::~PreferencesApply()
{
}

int PreferencesApply::handle_event()
{
	thread->apply_settings();
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
	thread->window->set_current_dialog(thread->text_to_category(get_text()));
	return 1;
}

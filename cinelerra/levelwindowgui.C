#include "edl.h"
#include "edlsession.h"
#include "levelwindow.h"
#include "levelwindowgui.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "preferences.h"
#include "theme.h"

LevelWindowGUI::LevelWindowGUI(MWindow *mwindow, LevelWindow *thread)
 : BC_Window(PROGRAM_NAME ": Levels", 
 	mwindow->session->lwindow_x, 
	mwindow->session->lwindow_y, 
	mwindow->session->lwindow_w, 
	mwindow->session->lwindow_h, 
	10,
	10,
	1,
	0, 
	1)
{
	this->thread = thread;
	this->mwindow = mwindow;
}

LevelWindowGUI::~LevelWindowGUI()
{
	delete panel;
}

int LevelWindowGUI::create_objects()
{
	mwindow->theme->draw_lwindow_bg(this);
	panel = new MeterPanel(mwindow, 
		this, 
		5, 
		5,
		get_h() - 10,
		mwindow->edl->session->audio_channels,
		1);
	panel->create_objects();
	return 0;
}


int LevelWindowGUI::resize_event(int w, int h)
{
	mwindow->session->lwindow_x = get_x();
	mwindow->session->lwindow_y = get_y();
	mwindow->session->lwindow_w = w;
	mwindow->session->lwindow_h = h;

	mwindow->theme->draw_lwindow_bg(this);

	panel->reposition_window(panel->x,
		panel->y,
		h - 10);

	BC_WindowBase::resize_event(w, h);
	return 1;
}

int LevelWindowGUI::translation_event()
{
	mwindow->session->lwindow_x = get_x();
	mwindow->session->lwindow_y = get_y();
	return 0;
}

int LevelWindowGUI::close_event()
{
	hide_window();
	mwindow->session->show_lwindow = 0;
	mwindow->gui->lock_window();
	mwindow->gui->mainmenu->show_lwindow->set_checked(0);
	mwindow->gui->unlock_window();
	mwindow->save_defaults();
	return 1;
}

int LevelWindowGUI::keypress_event()
{
	if(get_keypress() == 'w' || get_keypress() == 'W')
	{
		close_event();
		return 1;
	}
	return 0;
}


int LevelWindowGUI::reset_over()
{
	return 0;
}


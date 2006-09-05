#include "autos.h"
#include "bcsignals.h"
#include "cplayback.h"
#include "ctimebar.h"
#include "ctracking.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "cwindowtool.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "playtransport.h"
#include "preferences.h"
#include "theme.h"
#include "track.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportque.h"
#include "mwindow.h"



#include <unistd.h>



CWindow::CWindow(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
}


CWindow::~CWindow()
{
	delete playback_engine;
	delete playback_cursor;
}

int CWindow::create_objects()
{
	destination = mwindow->defaults->get("CWINDOW_DESTINATION", 0);

	gui = new CWindowGUI(mwindow, this);
    gui->create_objects();

	playback_engine = new CPlayback(mwindow, this, gui->canvas);

// Start command loop
	playback_engine->create_objects();
	gui->transport->set_engine(playback_engine);
	playback_cursor = new CTracking(mwindow, this);
	playback_cursor->create_objects();
    return 0;
}


void CWindow::show_window()
{
	gui->lock_window("CWindow::show_cwindow");
	gui->show_window();
	gui->raise_window();
	gui->flush();
	gui->unlock_window();

	gui->tool_panel->show_tool();
}

void CWindow::hide_window()
{
	gui->hide_window();
	gui->mwindow->session->show_cwindow = 0;
// Unlock in case MWindow is waiting for it.
	gui->unlock_window();

	gui->tool_panel->hide_tool();

	mwindow->gui->lock_window("CWindowGUI::close_event");
	mwindow->gui->mainmenu->show_cwindow->set_checked(0);
	mwindow->gui->unlock_window();
	mwindow->save_defaults();

	gui->lock_window("CWindow::hide_window");
}


Track* CWindow::calculate_affected_track()
{
	Track* affected_track = 0;
	for(Track *track = mwindow->edl->tracks->first;
		track;
		track = track->next)
	{
		if(track->data_type == TRACK_VIDEO &&
			track->record)
		{
			affected_track = track;
			break;
		}
	}
	return affected_track;
}

Auto* CWindow::calculate_affected_auto(Autos *autos, 
	int create,
	int *created,
	int redraw)
{
	Auto* affected_auto = 0;
	if(created) *created = 0;

	if(create)
	{
		int total = autos->total();
		affected_auto = autos->get_auto_for_editing();

// Got created
		if(total != autos->total())
		{
			if(created) *created = 1;
			if(redraw)
			{
				mwindow->gui->lock_window("CWindow::calculate_affected_auto");
				mwindow->gui->canvas->draw_overlays();
				mwindow->gui->canvas->flash();
				mwindow->gui->unlock_window();
			}
		}
	}
	else
	{
		affected_auto = autos->get_prev_auto(PLAY_FORWARD, affected_auto);
	}

	return affected_auto;
}



void CWindow::calculate_affected_autos(FloatAuto **x_auto,
	FloatAuto **y_auto,
	FloatAuto **z_auto,
	Track *track,
	int use_camera,
	int create_x,
	int create_y,
	int create_z)
{
	if(x_auto) (*x_auto) = 0;
	if(y_auto) (*y_auto) = 0;
	if(z_auto) (*z_auto) = 0;

	if(!track) return;

	if(use_camera)
	{
		if(x_auto) (*x_auto) = (FloatAuto*)calculate_affected_auto(
			track->automation->autos[AUTOMATION_CAMERA_X], create_x);
		if(y_auto) (*y_auto) = (FloatAuto*)calculate_affected_auto(
			track->automation->autos[AUTOMATION_CAMERA_Y], create_y);
		if(z_auto) (*z_auto) = (FloatAuto*)calculate_affected_auto(
			track->automation->autos[AUTOMATION_CAMERA_Z], create_z);
	}
	else
	{
		if(x_auto) (*x_auto) = (FloatAuto*)calculate_affected_auto(
			track->automation->autos[AUTOMATION_PROJECTOR_X], create_x);
		if(y_auto) (*y_auto) = (FloatAuto*)calculate_affected_auto(
			track->automation->autos[AUTOMATION_PROJECTOR_Y], create_y);
		if(z_auto) (*z_auto) = (FloatAuto*)calculate_affected_auto(
			track->automation->autos[AUTOMATION_PROJECTOR_Z], create_z);
	}
}





void CWindow::run()
{
	gui->run_window();
}

void CWindow::update(int position, 
	int overlays, 
	int tool_window, 
	int operation,
	int timebar)
{

	if(position)
	{
		gui->lock_window("CWindow::update 1");
		gui->slider->set_position();
		gui->unlock_window();

		playback_engine->que->send_command(CURRENT_FRAME, 
			CHANGE_NONE,
			mwindow->edl,
			1);
	}

	gui->lock_window("CWindow::update 2");


// Create tool window
	if(operation)
	{
		gui->set_operation(mwindow->edl->session->cwindow_operation);
	}


// Updated by video device.
	if(overlays && !position)
	{
		gui->canvas->draw_refresh();
	}

// Update tool parameters
// Never updated by someone else
	if(tool_window || position)
	{
		gui->update_tool();
	}

	if(timebar)
	{
		gui->timebar->update(1, 1);
	}

	if(!mwindow->edl->session->cwindow_scrollbars)
		gui->zoom_panel->update(AUTO_ZOOM);
	else
		gui->zoom_panel->update(mwindow->edl->session->cwindow_zoom);

	gui->canvas->update_zoom(mwindow->edl->session->cwindow_xscroll,
			mwindow->edl->session->cwindow_yscroll, 
			mwindow->edl->session->cwindow_zoom);
	gui->canvas->reposition_window(mwindow->edl, 
			mwindow->theme->ccanvas_x,
			mwindow->theme->ccanvas_y,
			mwindow->theme->ccanvas_w,
			mwindow->theme->ccanvas_h);




	gui->unlock_window();




}



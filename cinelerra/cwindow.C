#include "autos.h"
#include "bcsignals.h"
#include "cplayback.h"
#include "ctimebar.h"
#include "ctracking.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "defaults.h"
#include "edl.h"
#include "edlsession.h"
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

Auto* CWindow::calculate_affected_auto(Autos *autos, int create)
{
	Auto* affected_auto = 0;

	if(create)
	{
		int total = autos->total();
		affected_auto = autos->get_auto_for_editing();
		if(total != autos->total())
		{
			mwindow->gui->lock_window("CWindow::calculate_affected_auto");
			mwindow->gui->canvas->draw_overlays();
			mwindow->gui->canvas->flash();
			mwindow->gui->unlock_window();
		}
	}
	else
	{
		affected_auto = autos->get_prev_auto(PLAY_FORWARD, affected_auto);
	}

	return affected_auto;
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
//TRACE("CWindow::update 1");

	if(position)
	{
//printf("CWindow::update 2\n");
		gui->lock_window("CWindow::update 1");
		gui->slider->set_position();
		gui->unlock_window();
//printf("CWindow::update 2\n");
		playback_engine->que->send_command(CURRENT_FRAME, 
			CHANGE_NONE,
			mwindow->edl,
			1);
//printf("CWindow::update 3\n");
	}
//TRACE("CWindow::update 4");

	gui->lock_window("CWindow::update 2");


// Create tool window
	if(operation)
	{
		gui->set_operation(mwindow->edl->session->cwindow_operation);
	}

//TRACE("CWindow::update 5");

// Updated by video device.
	if(overlays && !position)
	{
		gui->canvas->draw_refresh();
	}

//TRACE("CWindow::update 5");
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

//TRACE("CWindow::update 6");
	if(!mwindow->edl->session->cwindow_scrollbars)
		gui->zoom_panel->update(AUTO_ZOOM);
	else
		gui->zoom_panel->update(mwindow->edl->session->cwindow_zoom);
//printf("CWindow::update 6\n");

	gui->canvas->update_zoom(mwindow->edl->session->cwindow_xscroll,
			mwindow->edl->session->cwindow_yscroll, 
			mwindow->edl->session->cwindow_zoom);
//printf("CWindow::update 6\n");
	gui->canvas->reposition_window(mwindow->edl, 
			mwindow->theme->ccanvas_x,
			mwindow->theme->ccanvas_y,
			mwindow->theme->ccanvas_w,
			mwindow->theme->ccanvas_h);

//printf("CWindow::update 6\n");



	gui->unlock_window();




//printf("CWindow::update 7\n");
}



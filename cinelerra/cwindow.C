#include "autos.h"
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

//printf("CWindow::create_objects 1\n");
// Start command loop
	playback_engine->create_objects();
//printf("CWindow::create_objects 1\n");
	gui->transport->set_engine(playback_engine);
//printf("CWindow::create_objects 1\n");
	playback_cursor = new CTracking(mwindow, this);
//printf("CWindow::create_objects 1\n");
	playback_cursor->create_objects();
//printf("CWindow::create_objects 2\n");
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
			mwindow->gui->lock_window();
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
//printf("CWindow::update 1\n");

	if(position)
	{
//printf("CWindow::update 2\n");
		gui->lock_window();
		gui->slider->set_position();
		gui->unlock_window();
//printf("CWindow::update 2\n");
		playback_engine->que->send_command(CURRENT_FRAME, 
			CHANGE_NONE,
			mwindow->edl,
			1);
//printf("CWindow::update 3\n");
	}
//sleep(1);

//printf("CWindow::update 4\n");


	gui->lock_window();



	if(operation)
	{
		gui->set_operation(mwindow->edl->session->cwindow_operation);
	}

//printf("CWindow::update 5\n");

// Updated by video device.
	if(overlays && !position)
	{
		gui->canvas->draw_refresh();
	}

//printf("CWindow::update 5\n");
// Never updated by someone else
	if(tool_window || position)
	{
		gui->update_tool();
	}

	if(timebar)
	{
		gui->timebar->update(1, 1);
	}

//printf("CWindow::update 6 %f\n", mwindow->edl->session->cwindow_zoom);
	if(!mwindow->edl->session->cwindow_scrollbars)
		gui->zoom_panel->update(AUTO_ZOOM);
	else
		gui->zoom_panel->update(mwindow->edl->session->cwindow_zoom);
//printf("CWindow::update 6 %f\n", mwindow->edl->session->cwindow_zoom);

	gui->canvas->update_zoom(mwindow->edl->session->cwindow_xscroll,
			mwindow->edl->session->cwindow_yscroll, 
			mwindow->edl->session->cwindow_zoom);
//printf("CWindow::update 6 %f\n", mwindow->edl->session->cwindow_zoom);
	gui->canvas->reposition_window(mwindow->edl, 
			mwindow->theme->ccanvas_x,
			mwindow->theme->ccanvas_y,
			mwindow->theme->ccanvas_w,
			mwindow->theme->ccanvas_h);

//printf("CWindow::update 6 %f\n", mwindow->edl->session->cwindow_zoom);



	gui->unlock_window();




//printf("CWindow::update 7 %f\n", mwindow->edl->session->cwindow_zoom);
}



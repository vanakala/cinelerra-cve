#include "bcsignals.h"
#include "cplayback.h"
#include "ctimebar.h"
#include "ctracking.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "mainclock.h"
#include "maincursor.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "trackcanvas.h"
#include "transportque.h"
#include "zoombar.h"

CTracking::CTracking(MWindow *mwindow, CWindow *cwindow)
 : Tracking(mwindow)
{
	this->cwindow = cwindow;
}

CTracking::~CTracking()
{
}

PlaybackEngine* CTracking::get_playback_engine()
{
	return cwindow->playback_engine;
}

int CTracking::start_playback(double new_position)
{
	mwindow->gui->cursor->playing_back = 1;

	Tracking::start_playback(new_position);
	return 0;
}

int CTracking::stop_playback()
{
	mwindow->gui->cursor->playing_back = 0;


	Tracking::stop_playback();
	return 0;
}

#define SCROLL_THRESHOLD 0


int CTracking::update_scroll(double position)
{
	int updated_scroll = 0;

	if(mwindow->edl->session->view_follows_playback)
	{
		double seconds_per_pixel = (double)mwindow->edl->local_session->zoom_sample / 
			mwindow->edl->session->sample_rate;
		double half_canvas = seconds_per_pixel * 
			mwindow->gui->canvas->get_w() / 2;
		double midpoint = mwindow->edl->local_session->view_start * 
			seconds_per_pixel +
			half_canvas;

		if(get_playback_engine()->command->get_direction() == PLAY_FORWARD)
		{
			double left_boundary = midpoint + SCROLL_THRESHOLD * half_canvas;
			double right_boundary = midpoint + half_canvas;

			if(position > left_boundary &&
				position < right_boundary)
			{
				int pixels = Units::to_int64((position - midpoint) * 
					mwindow->edl->session->sample_rate /
					mwindow->edl->local_session->zoom_sample);
				if(pixels) 
				{
					mwindow->move_right(pixels);
//printf("CTracking::update_scroll 1 %d\n", pixels);
					updated_scroll = 1;
				}
			}
		}
		else
		{
			double right_boundary = midpoint - SCROLL_THRESHOLD * half_canvas;
			double left_boundary = midpoint - half_canvas;

			if(position < right_boundary &&
				position > left_boundary && 
				mwindow->edl->local_session->view_start > 0)
			{
				int pixels = Units::to_int64((midpoint - position) * 
						mwindow->edl->session->sample_rate /
						mwindow->edl->local_session->zoom_sample);
				if(pixels) 
				{
					mwindow->move_left(pixels);
					updated_scroll = 1;
				}
			}
		}
	}

	return updated_scroll;
}

void CTracking::update_tracker(double position)
{
	int updated_scroll = 0;
// Update cwindow slider
//TRACE("CTracking::update_tracker 1");
	cwindow->gui->lock_window("CTracking::update_tracker 1");
//TRACE("CTracking::update_tracker 2");
	cwindow->gui->slider->update(position);

// This is going to boost the latency but we need to update the timebar
	cwindow->gui->timebar->draw_range();
	cwindow->gui->timebar->flash();
	cwindow->gui->unlock_window();
//TRACE("CTracking::update_tracker 3");

// Update mwindow cursor
	mwindow->gui->lock_window("CTracking::update_tracker 2");
//TRACE("CTracking::update_tracker 4");

	mwindow->edl->local_session->selectionstart = 
		mwindow->edl->local_session->selectionend = 
		position;

	updated_scroll = update_scroll(position);
//TRACE("CTracking::update_tracker 5");

	mwindow->gui->mainclock->update(position);
//TRACE("CTracking::update_tracker 5.5");
	mwindow->gui->patchbay->update();
//TRACE("CTracking::update_tracker 6");

	if(!updated_scroll)
	{
		mwindow->gui->cursor->update();
		mwindow->gui->zoombar->update();


		mwindow->gui->canvas->flash();
		mwindow->gui->flush();
	}
	mwindow->gui->unlock_window();
//TRACE("CTracking::update_tracker 7");

// Plugin GUI's hold lock on mwindow->gui here during user interface handlers.
	mwindow->update_plugin_guis();
//TRACE("CTracking::update_tracker 8");


	update_meters((int64_t)(position * mwindow->edl->session->sample_rate));
//TRACE("CTracking::update_tracker 9");
}

void CTracking::draw()
{
}

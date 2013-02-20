
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
#include "transportcommand.h"
#include "zoombar.h"

CTracking::CTracking(MWindow *mwindow, CWindow *cwindow)
 : Tracking(mwindow)
{
	this->cwindow = cwindow;
}

PlaybackEngine* CTracking::get_playback_engine()
{
	return cwindow->playback_engine;
}

void CTracking::start_playback(ptstime new_position)
{
	mwindow->gui->cursor->playing_back = 1;
	mwindow->edl->local_session->get_selections(selections);

	Tracking::start_playback(new_position);
}

void CTracking::stop_playback()
{
	ptstime epos;

	mwindow->gui->cursor->playing_back = 0;

	Tracking::stop_playback();

	epos = mwindow->edl->local_session->get_selectionstart();
	if(!PTSEQU(selections[0], selections[1]) &&
			(PTSEQU(epos, selections[1]) || PTSEQU(epos, selections[0])))
	{
		// restore highligt while reached end of highligt
		mwindow->edl->local_session->set_selection(selections[0], selections[1]);
		mwindow->gui->cursor->update();
	}
	selections[0] = selections[1] = 0;

}

#define SCROLL_THRESHOLD 0


int CTracking::update_scroll(ptstime position)
{
	int updated_scroll = 0;

	if(mwindow->edl->session->view_follows_playback)
	{
		ptstime seconds_per_pixel = mwindow->edl->local_session->zoom_time;
		ptstime half_canvas = seconds_per_pixel * 
			mwindow->gui->canvas->get_w() / 2;
		ptstime midpoint = mwindow->edl->local_session->view_start_pts +
			half_canvas;

		if(get_playback_engine()->command->get_direction() == PLAY_FORWARD)
		{
			ptstime left_boundary = midpoint + SCROLL_THRESHOLD * half_canvas;
			ptstime right_boundary = midpoint + half_canvas;

			if(position > left_boundary &&
				position < right_boundary)
			{
				int pixels = round((position - midpoint) /
					mwindow->edl->local_session->zoom_time);
				if(pixels) 
				{
					mwindow->move_right(pixels);
					updated_scroll = 1;
				}
			}
		}
		else
		{
			ptstime right_boundary = midpoint - SCROLL_THRESHOLD * half_canvas;
			ptstime left_boundary = midpoint - half_canvas;

			if(position < right_boundary &&
				position > left_boundary && 
				mwindow->edl->local_session->view_start_pts > 0)
			{
				int pixels = round((midpoint - position) /
						mwindow->edl->local_session->zoom_time);
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

void CTracking::update_tracker(ptstime position)
{
	int updated_scroll = 0;
// Update cwindow slider
	cwindow->gui->lock_window("CTracking::update_tracker 1");
	cwindow->gui->slider->update(position);

// This is going to boost the latency but we need to update the timebar
	cwindow->gui->timebar->update();
	cwindow->gui->unlock_window();

// Update mwindow cursor
	mwindow->gui->lock_window("CTracking::update_tracker 2");

	mwindow->edl->local_session->set_selection(position);

	updated_scroll = update_scroll(position);

	mwindow->gui->mainclock->update(position);
	mwindow->gui->patchbay->update();

	if(!updated_scroll)
	{
		mwindow->gui->cursor->update();
		mwindow->gui->zoombar->update_clocks();   // we just need to update clocks, not everything

		mwindow->gui->canvas->flash();
		mwindow->gui->flush();
	}
	mwindow->gui->unlock_window();

// Plugin GUI's hold lock on mwindow->gui here during user interface handlers.
	mwindow->update_plugin_guis();

	update_meters(position);
}

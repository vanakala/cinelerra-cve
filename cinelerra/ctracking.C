// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "cplayback.h"
#include "ctimebar.h"
#include "ctracking.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "levelwindow.h"
#include "levelwindowgui.h"
#include "localsession.h"
#include "mainclock.h"
#include "maincursor.h"
#include "mtimebar.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "trackcanvas.h"
#include "transportcommand.h"
#include "zoombar.h"

CTracking::CTracking(CWindow *cwindow)
 : Tracking()
{
	this->cwindow = cwindow;
	memset(selections, 0, sizeof(selections));
}

PlaybackEngine* CTracking::get_playback_engine()
{
	return cwindow->playback_engine;
}

void CTracking::start_playback(ptstime new_position)
{
	mwindow_global->gui->cursor->playing_back = 1;
	master_edl->local_session->get_selections(selections);

	Tracking::start_playback(new_position);
}

void CTracking::stop_playback()
{
	ptstime epos;

	mwindow_global->gui->cursor->playing_back = 0;

	Tracking::stop_playback();

	epos = master_edl->local_session->get_selectionstart();
	if(!PTSEQU(selections[0], selections[1]) &&
			(PTSEQU(epos, selections[1]) || PTSEQU(epos, selections[0])))
	{
		// restore highligt while reached end of highligt
		master_edl->local_session->set_selection(selections[0], selections[1]);
		mwindow_global->update_gui(WUPD_CURSOR);
	}
	selections[0] = selections[1] = 0;
	cwindow->gui->update_tool();
}

#define SCROLL_THRESHOLD 0


int CTracking::update_scroll(ptstime position)
{
	int updated_scroll = 0;
	ptstime seconds_per_pixel = master_edl->local_session->zoom_time;
	ptstime view_start = master_edl->local_session->view_start_pts;
	ptstime view_end = view_start +
		seconds_per_pixel * mwindow_global->gui->canvas->get_w();

	if(edlsession->view_follows_playback)
	{
		ptstime half_canvas = seconds_per_pixel *
			mwindow_global->gui->canvas->get_w()/ 2;
		ptstime midpoint = view_start + half_canvas;

		if(get_playback_engine()->command->get_direction() == PLAY_FORWARD &&
			master_edl->total_length() > view_end)
		{
			ptstime left_boundary = midpoint + SCROLL_THRESHOLD * half_canvas;
			ptstime right_boundary = midpoint + half_canvas;

			if(position > left_boundary &&
				position < right_boundary)
			{
				int pixels = round((position - midpoint) /
					master_edl->local_session->zoom_time);
				if(pixels) 
				{
					mwindow_global->move_right(pixels);
					updated_scroll = 1;
				}
			}
		}
		else if(view_start > 0)
		{
			ptstime right_boundary = midpoint - SCROLL_THRESHOLD * half_canvas;
			ptstime left_boundary = midpoint - half_canvas;

			if(position < right_boundary &&
				position > left_boundary && 
				master_edl->local_session->view_start_pts > 0)
			{
				int pixels = round((midpoint - position) /
						master_edl->local_session->zoom_time);
				if(pixels) 
				{
					mwindow_global->move_left(pixels);
					updated_scroll = 1;
				}
			}
		}
	}

	return updated_scroll;
}

void CTracking::update_tracker(ptstime position)
{
	int updated_scroll;
	int single_frame;
	int opts;

	master_edl->local_session->set_selection(position);
	single_frame = get_playback_engine()->command->single_frame();
// Update cwindow slider
	cwindow->gui->slider->update(position);

// This is going to boost the latency but we need to update the timebar
	cwindow->gui->timebar->update(!single_frame);

// Update mwindow cursor
	updated_scroll = update_scroll(position);

	opts = WUPD_PATCHBAY | WUPD_CLOCK | WUPD_PATCHBAY;

	if(!updated_scroll)
	{
		opts |= WUPD_CURSOR | WUPD_ZCLOCKS;
		if(single_frame)
			opts |= WUPD_TOGLIGHTS;
	}
	mwindow_global->update_gui(opts);

	master_edl->reset_plugins();
	update_meters(position);
}

void CTracking::update_meters(ptstime pts)
{
	double output_levels[MAXCHANNELS];
	double module_levels[MAXCHANNELS];

	if(get_playback_engine()->get_output_levels(output_levels, pts))
	{
		int n = get_playback_engine()->get_module_levels(module_levels, pts);

		cwindow->gui->meters->update(output_levels);
		mwindow_global->lwindow->gui->panel->update(output_levels);
		mwindow_global->gui->patchbay->update_meters(module_levels, n);
	}
}

void CTracking::stop_meters()
{
	cwindow->gui->meters->stop_meters();
	mwindow_global->gui->patchbay->stop_meters();
	mwindow_global->lwindow->gui->panel->stop_meters();
}

void CTracking::set_delays(ptstime over_delay, ptstime peak_delay)
{
	int over = round(over_delay * tracking_rate);
	int peak = round(peak_delay * tracking_rate);

	cwindow->gui->meters->set_delays(over, peak);
	mwindow_global->gui->patchbay->set_delays(over, peak);
	mwindow_global->lwindow->gui->panel->set_delays(over, peak);
}

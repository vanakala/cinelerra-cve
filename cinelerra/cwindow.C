// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "autos.h"
#include "bcsignals.h"
#include "cinelerra.h"
#include "cplayback.h"
#include "ctimebar.h"
#include "ctracking.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "cwindowtool.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "localsession.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "playbackengine.h"
#include "playtransport.h"
#include "preferences.h"
#include "theme.h"
#include "track.h"
#include "transportcommand.h"
#include "mwindow.h"

#include <unistd.h>


CWindow::CWindow()
 : Thread()
{
	destination = mwindow_global->defaults->get("CWINDOW_DESTINATION", 0);

	gui = new CWindowGUI(this);

	playback_engine = new CPlayback(this, gui->canvas);

// Start command loop
	gui->transport->set_engine(playback_engine);
	playback_cursor = new CTracking(this);
}

CWindow::~CWindow()
{
	delete playback_engine;
	delete playback_cursor;
}

void CWindow::show_window()
{
	gui->show_window();
	gui->raise_window();
	gui->tool_panel->show_tool();
}

void CWindow::hide_window()
{
	gui->hide_window();
	gui->tool_panel->hide_tool();
	mwindow_global->mark_cwindow_hidden();
}

Track* CWindow::calculate_affected_track()
{
	Track* affected_track = 0;
	for(Track *track = master_edl->first_track();
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

Auto* CWindow::calculate_affected_auto(int autoidx, ptstime pts,
	Track *track, int create, int *created, int redraw)
{
	Auto* affected_auto = 0;
	if(created)
		*created = 0;

	if(create)
	{
		int total = track->automation->total_autos(autoidx);
		affected_auto = track->automation->get_auto_for_editing(pts, autoidx);

// Got created
		if(total != track->automation->total_autos(autoidx))
		{
			if(created)
				*created = 1;
			if(redraw)
				mwindow_global->draw_canvas_overlays();
		}
	}
	else
	{
		affected_auto = track->automation->get_prev_auto(affected_auto, autoidx);
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
	int auto_x, auto_y, auto_z;

	if(x_auto)
		*x_auto = 0;
	if(y_auto)
		*y_auto = 0;
	if(z_auto)
		*z_auto = 0;

	if(!track) return;

	ptstime pts = master_edl->local_session->get_selectionstart(1);

	if(use_camera)
	{
		auto_x = AUTOMATION_CAMERA_X;
		auto_y = AUTOMATION_CAMERA_Y;
		auto_z = AUTOMATION_CAMERA_Z;
	}
	else
	{
		auto_x = AUTOMATION_PROJECTOR_X;
		auto_y = AUTOMATION_PROJECTOR_Y;
		auto_z = AUTOMATION_PROJECTOR_Z;
	}

	if(x_auto)
		*x_auto = (FloatAuto*)calculate_affected_auto(auto_x, pts,
			track, create_x);
	if(y_auto)
		*y_auto = (FloatAuto*)calculate_affected_auto(auto_y, pts,
			track, create_y);
	if(z_auto)
		*z_auto = (FloatAuto*)calculate_affected_auto(auto_z, pts,
			track, create_z);
}

void CWindow::run()
{
	gui->run_window();
}

void CWindow::update(int options)
{
	if(options & WUPD_SCROLLBARS)
	{
		gui->zoom_canvas(!edlsession->cwindow_scrollbars,
			edlsession->cwindow_zoom, 1);
		gui->canvas->update_scrollbars();
	}

	if(options & WUPD_POSITION)
	{
		gui->slider->set_position();

		playback_engine->send_command(CURRENT_FRAME);
	}

// Create tool window
	if(options & WUPD_OPERATION)
	{
		gui->set_operation(edlsession->cwindow_operation);
	}

	if(options & WUPD_OVERLAYS)
	{
		gui->canvas->update_guidelines();
		if(!(options & WUPD_POSITION))
			gui->canvas->draw_refresh();
	}

// Update tool parameters
// Never updated by someone else
	if(options & (WUPD_TOOLWIN | WUPD_POSITION))
	{
		gui->update_tool();
	}

	if(options & WUPD_TIMEBAR)
	{
		gui->timebar->update();
	}

	if(options & WUPD_ACHANNELS)
		gui->meters->set_meters(edlsession->audio_channels,
			edlsession->cwindow_meter);

	if(options & WUPD_ACHANNELS)
		gui->resize_event(mainsession->cwindow_w,
			mainsession->cwindow_h);
	else
		gui->canvas->reposition_window(master_edl,
			theme_global->ccanvas_x,
			theme_global->ccanvas_y,
			theme_global->ccanvas_w,
			theme_global->ccanvas_h);
}

GuideFrame *CWindow::new_guideframe(ptstime start, ptstime end, GuideFrame **gfp)
{
	if(!*gfp)
	{
		gui->canvas->lock_canvas("CWindow::new_guideframe");
		if(!*gfp)
			*gfp = gui->canvas->guidelines.append_frame(start, end);
		gui->canvas->unlock_canvas();
	}
	return *gfp;
}

void CWindow::delete_guideframe(GuideFrame **gfp)
{
	gui->canvas->lock_canvas("CWindow::delete_guideframe");
	if(*gfp)
	{
		gui->canvas->guidelines.remove(*gfp);
		*gfp = 0;
	}
	gui->canvas->unlock_canvas();
}

int CWindow::stop_playback()
{
	playback_engine->send_command(STOP);

	for(int i = 0; playback_engine->is_playing_back && i < 5; i++)
		usleep(50000);

	return playback_engine->is_playing_back;
}

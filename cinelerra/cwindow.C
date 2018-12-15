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
#include "mwindowgui.h"
#include "playbackengine.h"
#include "playtransport.h"
#include "preferences.h"
#include "theme.h"
#include "track.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportcommand.h"
#include "mwindow.h"

#include <unistd.h>


CWindow::CWindow(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;

	destination = mwindow->defaults->get("CWINDOW_DESTINATION", 0);

	gui = new CWindowGUI(mwindow, this);

	playback_engine = new CPlayback(mwindow, this, gui->canvas);

// Start command loop
	gui->transport->set_engine(playback_engine);
	playback_cursor = new CTracking(mwindow, this);
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
	gui->flush();
	gui->tool_panel->show_tool();
}

void CWindow::hide_window()
{
	gui->hide_window();
	gui->mwindow->session->show_cwindow = 0;
	gui->tool_panel->hide_tool();
	mwindow->gui->mainmenu->show_cwindow->set_checked(0);
}

Track* CWindow::calculate_affected_track()
{
	Track* affected_track = 0;
	for(Track *track = master_edl->tracks->first;
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
				mwindow->gui->canvas->draw_overlays();
				mwindow->gui->canvas->flash();
			}
		}
	}
	else
	{
		affected_auto = autos->get_prev_auto(affected_auto);
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

void CWindow::update(int options)
{
	if(options & WUPD_POSITION)
	{
		gui->slider->set_position();

		playback_engine->send_command(CURRENT_FRAME, master_edl);
	}

// Create tool window
	if(options & WUPD_OPERATION)
	{
		gui->set_operation(master_edl->session->cwindow_operation);
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
		gui->meters->set_meters(master_edl->session->audio_channels,
			master_edl->session->cwindow_meter);

	if(!master_edl->session->cwindow_scrollbars)
		gui->zoom_panel->update(_("Auto"));
	else
		gui->zoom_panel->update(master_edl->session->cwindow_zoom);

	gui->canvas->update_zoom(master_edl->session->cwindow_xscroll,
			master_edl->session->cwindow_yscroll,
			master_edl->session->cwindow_zoom);

	if(options & WUPD_ACHANNELS)
		gui->resize_event(mwindow->session->cwindow_w,
			mwindow->session->cwindow_h);
	else
		gui->canvas->reposition_window(master_edl,
			mwindow->theme->ccanvas_x,
			mwindow->theme->ccanvas_y,
			mwindow->theme->ccanvas_w,
			mwindow->theme->ccanvas_h);
}

GuideFrame *CWindow::new_guideframe(ptstime start, ptstime end)
{
	return gui->canvas->guidelines.append_frame(start, end);
}

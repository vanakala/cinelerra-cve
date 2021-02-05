// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "cinelerra.h"
#include "ctimebar.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "localsession.h"
#include "maincursor.h"
#include "mwindow.h"
#include "theme.h"


CTimeBar::CTimeBar(MWindow *mwindow, 
		CWindowGUI *gui,
		int x,
		int y,
		int w, 
		int h)
 : TimeBar(mwindow, 
		gui,
		x, 
		y,
		w,
		h)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

void CTimeBar::resize_event()
{
	reposition_window(mwindow->theme->ctimebar_x,
		mwindow->theme->ctimebar_y,
		mwindow->theme->ctimebar_w,
		mwindow->theme->ctimebar_h);
	update();
}

void CTimeBar::draw_time()
{
	get_edl_length();
	draw_range();
}

void CTimeBar::update_preview()
{
	gui->slider->set_position();
}

void CTimeBar::select_label(ptstime position)
{
	EDL *edl = master_edl;

	mwindow->stop_composer();

	position = master_edl->align_to_frame(position);

	if(shift_down())
	{
		if(position > edl->local_session->get_selectionend(1) / 2 + 
			edl->local_session->get_selectionstart(1) / 2)
		{
			edl->local_session->set_selectionend(position);
		}
		else
		{
			edl->local_session->set_selectionstart(position);
		}
	}
	else
	{
		edl->local_session->set_selectionstart(position);
		edl->local_session->set_selectionend(position);
	}

// Que the CWindow
	mwindow->cwindow->update(WUPD_POSITION | WUPD_TIMEBAR);
	mwindow->update_gui(WUPD_CANVINCR | WUPD_TIMEBAR | WUPD_PATCHBAY | WUPD_CLOCK);
	mwindow->update_plugin_guis();
}

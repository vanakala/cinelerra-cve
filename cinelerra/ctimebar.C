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


CTimeBar::CTimeBar(CWindowGUI *gui,
		int x,
		int y,
		int w, 
		int h)
 : TimeBar(gui,
		x, 
		y,
		w,
		h)
{
	this->gui = gui;
}

void CTimeBar::resize_event()
{
	reposition_window(theme_global->ctimebar_x,
		theme_global->ctimebar_y,
		theme_global->ctimebar_w,
		theme_global->ctimebar_h);
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

	mwindow_global->stop_composer();

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
	mwindow_global->cwindow->update(WUPD_POSITION | WUPD_TIMEBAR);
	mwindow_global->update_gui(WUPD_CANVINCR | WUPD_TIMEBAR | WUPD_PATCHBAY | WUPD_CLOCK);
	mwindow_global->update_plugin_guis();
}

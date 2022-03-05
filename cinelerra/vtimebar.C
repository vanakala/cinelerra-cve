// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "edl.h"
#include "localsession.h"
#include "mwindow.h"
#include "theme.h"
#include "vtimebar.h"
#include "vwindow.h"
#include "vwindowgui.h"


VTimeBar::VTimeBar(VWindowGUI *gui,
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

void VTimeBar::resize_event()
{
	reposition_window(theme_global->vtimebar_x,
		theme_global->vtimebar_y,
		theme_global->vtimebar_w,
		theme_global->vtimebar_h);
	update();
}

EDL* VTimeBar::get_edl()
{
	return vwindow_edl;
}

void VTimeBar::draw_time()
{
	draw_range();
}

void VTimeBar::update_preview()
{
	gui->slider->set_position();
}

void VTimeBar::select_label(ptstime position)
{
	gui->transport->handle_transport(STOP);

	position = vwindow_edl->align_to_frame(position);

	if(shift_down())
	{
		if(position > vwindow_edl->local_session->get_selectionend(1) / 2 +
				vwindow_edl->local_session->get_selectionstart(1) / 2)
			vwindow_edl->local_session->set_selectionend(position);
		else
			vwindow_edl->local_session->set_selectionstart(position);
	}
	else
		vwindow_edl->local_session->set_selection(position);

	mwindow_global->vwindow->update_position(0, 1);
}

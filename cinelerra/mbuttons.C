// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "cplayback.h"
#include "cwindow.h"
#include "editpanel.h"
#include "edl.inc"
#include "mbuttons.h"
#include "mwindow.h"
#include "playbackengine.inc"
#include "playtransport.h"
#include "theme.h"

MButtons::MButtons()
 : BC_SubWindow(theme_global->mbuttons_x,
	theme_global->mbuttons_y,
	theme_global->mbuttons_w,
	theme_global->mbuttons_h)
{
	transport = 0;
	edit_panel = 0;
}

MButtons::~MButtons()
{
	delete transport;
	delete edit_panel;
}

void MButtons::show()
{
	int x = 3, y = 0;
	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	transport = new MainTransport(this, x, y);
	transport->set_engine(mwindow_global->cwindow->playback_engine);
	x += transport->get_w();
	x += theme_global->mtransport_margin;

	edit_panel = new MainEditing(this, x, y);
	flash();
}

void MButtons::resize_event()
{
	reposition_window(theme_global->mbuttons_x,
		theme_global->mbuttons_y,
		theme_global->mbuttons_w,
		theme_global->mbuttons_h);
	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	flash();
}

int MButtons::keypress_event()
{
	return transport->keypress_event();
}

void MButtons::update()
{
	edit_panel->update();
}


MainTransport::MainTransport(MButtons *mbuttons, int x, int y)
 : PlayTransport(mbuttons, x, y)
{
}

void MainTransport::goto_start()
{
	handle_transport(REWIND, 1);
	mwindow_global->goto_start();
}

void MainTransport::goto_end()
{
	handle_transport(GOTO_END, 1);
	mwindow_global->goto_end();
}

EDL *MainTransport::get_edl()
{
	return master_edl;
}


MainEditing::MainEditing(MButtons *mbuttons, int x, int y)
 : EditPanel(mbuttons, x, y,
		EDTP_EDITING_MODE | EDTP_KEYFRAME | EDTP_COPY | EDTP_PASTE
			| EDTP_UNDO | EDTP_FIT | EDTP_LOCKLABELS | EDTP_LABELS 
			| EDTP_TOCLIP | EDTP_MWINDOW | EDTP_CUT, 0)
{
	this->mbuttons = mbuttons;
}

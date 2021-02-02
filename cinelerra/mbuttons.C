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

MButtons::MButtons(MWindow *mwindow)
 : BC_SubWindow(mwindow->theme->mbuttons_x, 
	mwindow->theme->mbuttons_y,
	mwindow->theme->mbuttons_w, 
	mwindow->theme->mbuttons_h)
{
	this->mwindow = mwindow;
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
	transport = new MainTransport(mwindow, this, x, y);
	transport->set_engine(mwindow->cwindow->playback_engine);
	x += transport->get_w();
	x += mwindow->theme->mtransport_margin;

	edit_panel = new MainEditing(mwindow, this, x, y);
	flash();
}

void MButtons::resize_event()
{
	reposition_window(mwindow->theme->mbuttons_x, 
		mwindow->theme->mbuttons_y, 
		mwindow->theme->mbuttons_w, 
		mwindow->theme->mbuttons_h);
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


MainTransport::MainTransport(MWindow *mwindow, MButtons *mbuttons, int x, int y)
 : PlayTransport(mwindow, mbuttons, x, y)
{
}

void MainTransport::goto_start()
{
	handle_transport(REWIND, 1);
	mwindow->goto_start();
}

void MainTransport::goto_end()
{
	handle_transport(GOTO_END, 1);
	mwindow->goto_end();
}

EDL *MainTransport::get_edl()
{
	return master_edl;
}


MainEditing::MainEditing(MWindow *mwindow, MButtons *mbuttons, int x, int y)
 : EditPanel(mwindow, 
		mbuttons, 
		x, 
		y,
		EDTP_EDITING_MODE | EDTP_KEYFRAME | EDTP_COPY | EDTP_PASTE
			| EDTP_UNDO | EDTP_FIT | EDTP_LOCKLABELS | EDTP_LABELS 
			| EDTP_TOCLIP | EDTP_MWINDOW | EDTP_CUT,
		0)
{
	this->mwindow = mwindow;
	this->mbuttons = mbuttons;
}

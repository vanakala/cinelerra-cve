
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

#include "cplayback.h"
#include "cwindow.h"
#include "editpanel.h"
#include "filexml.h"
#include "keys.h"
#include "localsession.h"
#include "mbuttons.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "playtransport.h"
#include "preferences.h"
#include "mainsession.h"
#include "theme.h"
#include "tracks.h"

MButtons::MButtons(MWindow *mwindow, MWindowGUI *gui)
 : BC_SubWindow(mwindow->theme->mbuttons_x, 
	mwindow->theme->mbuttons_y,
	mwindow->theme->mbuttons_w, 
	mwindow->theme->mbuttons_h)
{
	this->gui = gui;
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

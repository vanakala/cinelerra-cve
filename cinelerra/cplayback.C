
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
#include "ctracking.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playtransport.h"
#include "trackcanvas.h"
#include "transportque.h"

// Playback engine for composite window

CPlayback::CPlayback(MWindow *mwindow, CWindow *cwindow, Canvas *output)
 : PlaybackEngine(mwindow, output)
{
	this->cwindow = cwindow;
}

int CPlayback::create_render_engine()
{
	return PlaybackEngine::create_render_engine();
}

void CPlayback::init_cursor()
{
	mwindow->gui->lock_window("CPlayback::init_cursor");
	mwindow->gui->canvas->deactivate();
	mwindow->gui->unlock_window();
	cwindow->playback_cursor->start_playback(tracking_position);
}

void CPlayback::stop_cursor()
{
	cwindow->playback_cursor->stop_playback();
}


int CPlayback::brender_available(long position)
{
	return mwindow->brender_available(position);
}

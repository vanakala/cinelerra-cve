// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "cplayback.h"
#include "ctracking.h"
#include "cwindow.h"
#include "edl.h"
#include "mwindow.h"

// Playback engine for composite window

CPlayback::CPlayback(CWindow *cwindow, Canvas *output)
 : PlaybackEngine(output)
{
	this->cwindow = cwindow;
	edl = master_edl;
}

void CPlayback::init_cursor()
{
	mwindow_global->deactivate_canvas();
	cwindow->playback_cursor->start_playback(get_tracking_position());
}

void CPlayback::stop_cursor()
{
	cwindow->playback_cursor->stop_playback();

	if(is_playing_back)
		mwindow_global->activate_canvas();
	master_edl->reset_plugins();
}

int CPlayback::brender_available(ptstime position)
{
	return mwindow_global->brender_available(position);
}

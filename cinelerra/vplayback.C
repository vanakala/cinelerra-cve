// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "vplayback.h"
#include "vtracking.h"
#include "vwindow.h"

// Playback engine for viewer

VPlayback::VPlayback(VWindow *vwindow, Canvas *output)
 : PlaybackEngine(output)
{
	this->vwindow = vwindow;
	edl = vwindow_edl;
}

void VPlayback::init_cursor()
{
	vwindow->playback_cursor->start_playback(get_tracking_position());
}

void VPlayback::stop_cursor()
{
	vwindow->playback_cursor->stop_playback();
}



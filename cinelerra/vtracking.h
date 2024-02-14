// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef VTRACKING_H
#define VTRACKING_H

#include "tracking.h"
#include "playbackengine.inc"
#include "vwindow.inc"

class VTracking : public Tracking
{
public:
	VTracking(VWindow *vwindow);

	PlaybackEngine* get_playback_engine();
	void start_playback(ptstime new_position);
	void stop_playback();
	void update_tracker(ptstime position);
	void update_meters(ptstime pts);
	void stop_meters();
	void set_delays(ptstime over_delay, ptstime peak_delay);

	VWindow *vwindow;
};

#endif

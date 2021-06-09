// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef CPLAYBACKCURSOR_H
#define CPLAYBACKCURSOR_H

#include "cwindow.inc"
#include "tracking.h"
#include "playbackengine.inc"

class CTracking : public Tracking
{
public:
	CTracking(CWindow *cwindow);

	PlaybackEngine* get_playback_engine();
	void update_tracker(ptstime position);
// Move Track Canvas left or right to track playback
	int update_scroll(ptstime position);

// Move sliders and insertion point to track playback
	void start_playback(ptstime new_position);
	void stop_playback();

	void update_meters(ptstime pts);
	void stop_meters();
	void set_delays(float over_delay, float peak_delay);

private:
	CWindow *cwindow;
	ptstime selections[4];
};

#endif

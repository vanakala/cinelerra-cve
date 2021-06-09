// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef TRACKING_H
#define TRACKING_H

#include "bctimer.h"
#include "condition.inc"
#include "datatype.h"
#include "playbackengine.inc"
#include "thread.h"

class Tracking : public Thread
{
public:
	Tracking();
	virtual ~Tracking();

	virtual void start_playback(ptstime new_position);
	virtual void stop_playback();

// Called by the tracker to get the current position
	virtual PlaybackEngine* get_playback_engine() = 0;
	ptstime get_tracking_position();
// Update position displayed
	virtual void update_tracker(ptstime position) {};
// Update meters
	virtual void update_meters(ptstime pts) {};
	virtual void stop_meters() {};
	virtual void set_delays(float over_delay, float peak_delay) {};

	void run();

	int state;
	int view_follows_playback;
// Delay until startup
	Condition *startup_lock;
	ptstime last_position;
	int follow_loop;
	int reverse;
	int double_speed;
	Timer timer;
// Pixel of last drawn cursor
	int pixel;
// Cursor is visible
	int visible;
protected:
	int tracking_rate;
};

#endif

// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef RENDERBASE_H
#define RENDERBASE_H

#include "condition.inc"
#include "edl.inc"
#include "plugin.inc"
#include "renderbase.inc"
#include "renderengine.h"
#include "thread.h"
#include "track.inc"
#include "trackrender.inc"

class RenderBase : public Thread
{
public:
	RenderBase(RenderEngine *renderengine, EDL *edl);

	void arm_command();
	void start_command();
	virtual void run();
// advance playback position
// returns NZ when next frame must be flashed
	int advance_position(ptstime duration);
// Check that shared plugin is ready for rendering
	int is_shared_ready(Plugin *plugin, ptstime pts);
// Mark shared plugin rendered
	void shared_done(Plugin *plugin);

protected:
	virtual void init_frames() {};

	RenderEngine *renderengine;
	EDL *edl;
	ptstime render_pts;
	ptstime render_start;
	ptstime render_end;
	int render_direction;
	int render_loop;
	int render_single;
	int render_realtime;
	int last_playback;
	double render_speed;
	Condition *start_lock;

private:
	void adjust_next_plugin(Track *track, Plugin *plugin, int ix);
};

#endif

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


class RefPlugin
{
public:
	RefPlugin()
	{
		plugin = 0;
		track = 0;
		paired = 0;
	};

	RefPlugin(Plugin *plugin, Track *track);

	RefPlugin& operator=(RefPlugin &that)
	{
		plugin = that.plugin;
		track = that.track;
		paired = that.paired;
		return *this;
	};

	int operator==(RefPlugin &that)
	{
		return plugin == that.plugin && track == that.track;
	}

	int operator!=(RefPlugin &that)
	{
		return plugin != that.plugin || track != that.track;
	}

	Plugin *shared_master();
	void dump(int indent = 0);

	Plugin *plugin;
	Track *track;
	int paired;
};

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
// Check shared plugins and tracks
	static Plugin *check_multichannel_plugins();
	static int check_multichannel(Plugin *plugin);
// Returns 1 if plugin is possinle on track position ix_on_track
	static int multichannel_possible(Track *track, int ix_on_track,
		ptstime start, ptstime end,
		Plugin *shared);

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

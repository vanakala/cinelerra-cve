// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "bcsignals.h"
#include "condition.h"
#include "edl.h"
#include "plugin.h"
#include "renderbase.h"
#include "renderengine.h"
#include "thread.h"
#include "track.h"
#include "tracks.h"
#include "trackrender.h"
#include "transportcommand.h"

RenderBase::RenderBase(RenderEngine *renderengine, EDL *edl)
 : Thread(THREAD_SYNCHRONOUS)
{
	this->renderengine = renderengine;
	this->edl = edl;
	start_lock = new Condition(0, "RenderBase::start_lock");
}

void RenderBase::arm_command()
{
	render_pts = renderengine->command.playbackstart;
	render_start = renderengine->command.start_position;
	render_end = renderengine->command.end_position;
	render_direction = renderengine->command.get_direction();
	render_loop = renderengine->command.loop_playback;
	render_single = renderengine->command.single_frame();
	render_realtime = renderengine->command.realtime;
	render_speed = renderengine->command.get_speed();
	last_playback = 0;
	init_frames();
}

void RenderBase::start_command()
{
	if(render_realtime)
	{
		Thread::start();
		start_lock->lock("RenderBase::start_command");
	}
}

int RenderBase::advance_position(ptstime duration)
{
	last_playback = 0;
	if(render_direction == PLAY_FORWARD)
	{
		if(render_pts + duration > render_end - FRAME_ACCURACY)
		{
			duration = render_end - render_pts;
			last_playback = 1;
		}
		render_pts += duration;
	}
	else
	{
		if(render_pts - duration <= render_start - FRAME_ACCURACY)
		{
			duration = render_pts - render_start;
			last_playback = 1;
		}
		render_pts -= duration;
	}
	if(last_playback && render_loop)
	{
		last_playback = 0;
		if(render_direction == PLAY_FORWARD)
			render_pts = render_start;
		else
			render_pts = render_end;
		return 1;
	}
	return 0;
}

void RenderBase::run()
{
	start_lock->unlock();
}

int RenderBase::is_shared_ready(Plugin *plugin, ptstime pts)
{
	int pcount, ncount;
	int plugin_type = plugin->track->data_type;

	pcount = ncount = 0;
	for(Track *track = edl->tracks->first; track; track = track->next)
	{
		if(track->data_type != plugin_type || track->renderer->is_muted(pts))
			continue;
		for(int i = 0; i < track->plugins.total; i++)
		{
			Plugin *current = track->plugins.values[i];

			if(current->plugin_type == PLUGIN_SHAREDPLUGIN &&
				current->shared_plugin == plugin &&
				current->on)
			{
				pcount++;

				if(track->renderer && track->renderer->next_plugin &&
						track->renderer->next_plugin->shared_plugin == plugin)
					ncount++;
			}
		}
	}
	return pcount == ncount;
}

void RenderBase::shared_done(Plugin *plugin)
{
	int plugin_type = plugin->track->data_type;

	for(Track *track = edl->tracks->first; track; track = track->next)
	{
		if(track->data_type != plugin_type || !track->renderer)
			continue;

		for(int i = 0; i < track->plugins.total; i++)
		{
			Plugin *current = track->plugins.values[i];

			if(current == plugin)
				adjust_next_plugin(track, plugin, i);

			if(current->plugin_type == PLUGIN_SHAREDPLUGIN &&
					current->shared_plugin == plugin &&
					current->on)
			{
				if(track->renderer->next_plugin &&
						track->renderer->next_plugin->shared_plugin == plugin)
					adjust_next_plugin(track, track->renderer->next_plugin, i);
			}
		}
	}
}

void RenderBase::adjust_next_plugin(Track *track, Plugin *plugin, int ix)
{
	if(track->renderer->next_plugin == plugin)
	{
		if(ix < track->plugins.total - 1)
			track->renderer->next_plugin = track->plugins.values[ix + 1];
		else
			track->renderer->next_plugin = 0;
	}
}

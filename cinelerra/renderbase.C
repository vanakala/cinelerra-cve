
/*
 * CINELERRA
 * Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>
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

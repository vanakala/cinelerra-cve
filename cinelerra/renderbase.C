// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "bcsignals.h"
#include "condition.h"
#include "edl.h"
#include "plugin.h"
#include "pluginserver.h"
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

Plugin *RenderBase::check_multichannel_plugins()
{
	ArrayList<struct RefPlugin>mc_plugins;
	RefPlugin *ref;

// FixIt: shared tracks are missing
	for(Track *track = master_edl->tracks->last; track; track = track->previous)
	{
		for(int i = 0; i < track->plugins.total; i++)
		{
			Plugin *plugin = track->plugins.values[i];

			if(plugin->plugin_type == PLUGIN_SHAREDMODULE &&
					plugin->shared_track)
			{
				Track *reftrack = plugin->shared_track;

				for(int k = 0; k < reftrack->plugins.total; k++)
				{
					Plugin *cur = reftrack->plugins.values[k];

					if(check_multichannel(cur))
					{
						RefPlugin new_ref(cur, track);
						mc_plugins.append(new_ref);
					}
				}
			}

			if(check_multichannel(plugin))
			{
				RefPlugin new_ref(plugin, track);
				mc_plugins.append(new_ref);
			}
		}
	}

	if(mc_plugins.total < 2)
		return 0;

// Remove single plugins
	for(int i = 0; i < mc_plugins.total - 1; i++)
	{
		if(mc_plugins.values[i].track != mc_plugins.values[i + 1].track)
		{
			mc_plugins.remove(mc_plugins.values[i]);
			i--;
		}
		else for(int j = i + 1; j < mc_plugins.total; j++)
		{
			if(mc_plugins.values[i].track != mc_plugins.values[j].track)
			{
				i = j - 1;
				break;
			}
		}
	}
	if(mc_plugins.total < 2)
		return 0;

	int k = mc_plugins.total - 1;
// Remove the last single line
	if(mc_plugins.values[k].track != mc_plugins.values[k - 1].track)
		mc_plugins.remove(mc_plugins.values[k]);

// One track with multiple same plugins
	for(int i = 0; i < mc_plugins.total; i++)
	{
		RefPlugin *crp = &mc_plugins.values[i];
		Plugin *mp = crp->shared_master();

		for(int j = i + 1; j < mc_plugins.total; j++)
		{
			if(crp->track != mc_plugins.values[j].track)
				break;

			Plugin *cp = mc_plugins.values[j].shared_master();

			if(cp == mp)
				return crp->plugin;
		}
	}
// Count the number of tracks
	Track *tp = mc_plugins.values[0].track;
	int tc = 1;

	for(int i = 1; i < mc_plugins.total; i++)
	{
		if(tp != mc_plugins.values[i].track)
		{
			tc++;
			tp = mc_plugins.values[i].track;
		}
	}
	if(tc < 2)
		return 0;

// Remove plugins what are single or do not overlap
	for(int i = 0; i < mc_plugins.total; i++)
	{
		RefPlugin *crp = &mc_plugins.values[i];

		if(crp->paired)
			continue;

		Plugin *mp = crp->shared_master();

		for(int j = i + 1; j < mc_plugins.total; j++)
		{
			if(crp->track == mc_plugins.values[j].track ||
					mc_plugins.values[j].paired)
				continue;

			Plugin *cp = mc_plugins.values[j].shared_master();

			if(mp == cp)
			{
				crp->paired = 1;
				mc_plugins.values[j].paired = 1;
			}
		}
	}

	for(int i = 0; i < mc_plugins.total; i++)
	{
		if(!mc_plugins.values[i].paired)
		{
			mc_plugins.remove(mc_plugins.values[i]);
			i--;
		}
	}

// More than 1 plugin share tracks
// Number the plugins on track
	int count;
	tp = 0;
	for(int i = 0; i < mc_plugins.total; i++)
	{
		if(tp != mc_plugins.values[i].track)
		{
			count = 1;
			tp = mc_plugins.values[i].track;
		}
		mc_plugins.values[i].paired = count++;
	}

	for(int i = 0; i < mc_plugins.total; i++)
	{
		RefPlugin *crp = &mc_plugins.values[i];
		Plugin *mp = crp->shared_master();

		for(int j = i + 1; j < mc_plugins.total; j++)
		{
			if(crp->track == mc_plugins.values[j].track)
				continue;

			Plugin *cp = mc_plugins.values[j].shared_master();

			if(mp == cp)
			{
				if(mc_plugins.values[j].paired < crp->paired)
					return mc_plugins.values[j].plugin;
			}
		}
	}
	return 0;
}

int RenderBase::check_multichannel(Plugin *plugin)
{
	return (plugin->plugin_type == PLUGIN_STANDALONE &&
		plugin->plugin_server && plugin->plugin_server->multichannel) ||
		(plugin->shared_plugin &&
		plugin->shared_plugin->plugin_server &&
		plugin->shared_plugin->plugin_server->multichannel);
}


RefPlugin::RefPlugin(Plugin *plugin, Track *track)
{
	this->plugin = plugin;
	this->track = track;
	paired = 0;
}

Plugin *RefPlugin::shared_master()
{
	if(plugin->shared_plugin)
		return plugin->shared_plugin;
	return plugin;
}

void RefPlugin::dump(int indent)
{
	char string[BCTEXTLEN];

	printf("%*sRefPlugin %p %3d dump:\n", indent, "", this, paired);
	indent++;
	printf("%*s%p '%s'", indent, "", track, track->title);
	plugin->calculate_title(string);
	printf(" %p '%s'\n", plugin, string);
}

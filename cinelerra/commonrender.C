
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#include "auto.h"
#include "bcsignals.h"
#include "cache.h"
#include "commonrender.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "intautos.h"
#include "localsession.h"
#include "mainsession.h"
#include "module.h"
#include "mwindow.h"
#include "patchbay.h"
#include "playabletracks.h"
#include "preferences.h"
#include "renderengine.h"
#include "track.h"
#include "tracks.h"
#include "transportcommand.h"
#include "virtualconsole.h"

CommonRender::CommonRender(RenderEngine *renderengine)
 : Thread(THREAD_SYNCHRONOUS)
{
	this->renderengine = renderengine;
	reset_parameters();
	start_lock = new Condition(0, "CommonRender::start_lock");
}

CommonRender::~CommonRender()
{
	delete_vconsole();
	if(modules)
	{
		for(int i = 0; i < total_modules; i++)
			delete modules[i];
		delete [] modules;
	}
	delete start_lock;
}

void CommonRender::reset_parameters()
{
	total_modules = 0;
	modules = 0;
	vconsole = 0;
	done = 0;
	interrupt = 0;
	last_playback = 0;
	asynchronous = 0;
	restart_plugins = 0;
}

void CommonRender::arm_command()
{
	ptstime temp_length = fromunits(1);

	current_postime = renderengine->command->playbackstart;

	init_output_buffers();

	last_playback = 0;
	if(test_reconfigure(temp_length))
	{
		restart_playback();
	}
	else
	{
		vconsole->start_playback();
	}

	done = 0;
	interrupt = 0;
	last_playback = 0;
	restart_plugins = 0;
}



void CommonRender::create_modules()
{
// Create a module for every track, playable or not
	Track *current = renderengine->edl->tracks->first;
	int module = 0;

	if(!modules)
	{
		total_modules = get_total_tracks();
		modules = new Module*[total_modules];
		memset(modules, 0, sizeof(Module*) * total_modules);

		for(module = 0; module < total_modules && current; current = NEXT)
		{
			if(current->data_type == data_type)
			{
				modules[module] = new_module(current);
				modules[module]->create_objects();
				module++;
			}
		}
	}
	else
// Update changes in plugins for existing modules
	{
		for(module = 0; module < total_modules; module++)
		{
			if(modules[module])
				modules[module]->create_objects();
		}
	}
}

void CommonRender::start_plugins()
{
	if(restart_plugins)
	{
		for(int i = 0; i < total_modules; i++)
		{
			modules[i]->render_init();
		}
	}
}

void CommonRender::stop_plugins()
{
	for(int i = 0; i < total_modules; i++)
	{
		modules[i]->render_stop();
	}
}

int CommonRender::test_reconfigure(ptstime &length)
{
	if(!vconsole) return 1;
	if(!modules) return 1;

	return vconsole->test_reconfigure(length, last_playback);
}


void CommonRender::build_virtual_console()
{
// Create new virtual console object
	if(!vconsole)
	{
		vconsole = new_vconsole_object();
	}

// Create nodes
	vconsole->create_objects();
}

void CommonRender::start_command()
{
	if(renderengine->command->realtime)
	{
		Thread::start();
		start_lock->lock("CommonRender::start_command");
	}
}

void CommonRender::restart_playback()
{
	delete_vconsole();
	create_modules();
	build_virtual_console();
	start_plugins();

	done = 0;
	interrupt = 0;
	last_playback = 0;
	restart_plugins = 0;
}

void CommonRender::delete_vconsole()
{
	if(vconsole) delete vconsole;
	vconsole = 0;
}

void CommonRender::get_boundaries(ptstime &current_render_duration, ptstime min_duration)
{
	if(renderengine->command->single_frame())
	{
		last_playback = 1;
		return;
	}

	ptstime start_position = renderengine->command->start_position;
	ptstime end_position = renderengine->command->end_position;
	int direction = renderengine->command->get_direction();

// test absolute boundaries if no loop
	if(!renderengine->edl->local_session->loop_playback)
	{
		if(direction == PLAY_FORWARD)
		{
			if(current_postime + current_render_duration >= end_position)
			{
				last_playback = 1;
				current_render_duration = end_position - current_postime;
			}
		}
// reverse playback
		else
		{
			if(current_postime - current_render_duration <= start_position)
			{
				last_playback = 1;
				current_render_duration = current_postime - start_position;
			}
		}
	}
	else
// test against loop boundaries
	{
		ptstime loop_pts;
		if(renderengine->command->get_direction() == PLAY_FORWARD)
		{
			loop_pts = renderengine->edl->local_session->loop_end;

			if(current_postime + current_render_duration > loop_pts)
				current_render_duration = loop_pts - current_postime;
		}
		else
		{
			loop_pts = renderengine->edl->local_session->loop_start;
			if(current_postime - current_render_duration < loop_pts)
				current_render_duration = current_postime - loop_pts;
		}
	}

	if(current_render_duration < min_duration) 
		current_render_duration = min_duration;
}

void CommonRender::run()
{
	start_lock->unlock();
}


CommonRender::CommonRender(MWindow *mwindow, RenderEngine *renderengine)
 : Thread()
{
	this->mwindow = mwindow;
	this->renderengine = renderengine;
	current_postime = 0;
	interrupt = 0;
	done = 0;
	last_playback = 0;
	vconsole = 0;
	asynchronous = 1;
}

int CommonRender::wait_for_completion()
{
	join();
}


int CommonRender::advance_position(ptstime current_pts, ptstime current_render_duration)
{
	int result = 0;

// advance the playback position
	if(renderengine->command->get_direction() == PLAY_REVERSE)
		current_postime = current_pts - current_render_duration;
	else
		current_postime = current_pts + current_render_duration;

// test loop again
	if(renderengine->edl->local_session->loop_playback)
	{
		ptstime loop_end = renderengine->edl->local_session->loop_end;
		ptstime loop_start = renderengine->edl->local_session->loop_start;
		if(renderengine->command->get_direction() == PLAY_REVERSE)
		{
			if(current_postime <= loop_start)
			{
				current_postime = loop_end;
				result = 1;
			}
		}
		else
		{
			if(current_postime >= loop_end)
			{
				current_postime = loop_start;
				result = 1;
			}
		}
	}
	return result;
}

posnum CommonRender::tounits(ptstime position, int round)
{
	return (posnum)position;
}

ptstime CommonRender::fromunits(posnum position)
{
	return (ptstime)position;
}

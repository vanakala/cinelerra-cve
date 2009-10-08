
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
#include "patch.h"
#include "playabletracks.h"
#include "preferences.h"
#include "renderengine.h"
#include "track.h"
#include "tracks.h"
#include "transportque.h"
#include "virtualconsole.h"

CommonRender::CommonRender(RenderEngine *renderengine)
 : Thread(1, 0, 0)
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
	int64_t temp_length = 1;

	current_position = tounits(renderengine->command->playbackstart, 0);

	init_output_buffers();

	last_playback = 0;
	if(test_reconfigure(current_position, temp_length))
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
			modules[module]->create_objects();
		}
	}
}

void CommonRender::start_plugins()
{
// Only start if virtual console was created
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

int CommonRender::test_reconfigure(int64_t position, int64_t &length)
{
	if(!vconsole) return 1;
	if(!modules) return 1;
	
	return vconsole->test_reconfigure(position, length, last_playback);
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
		Thread::set_realtime(renderengine->edl->session->real_time_playback &&
			data_type == TRACK_AUDIO);
		Thread::start();
		start_lock->lock("CommonRender::start_command");
	}
}

int CommonRender::restart_playback()
{
	delete_vconsole();
	create_modules();
	build_virtual_console();
	start_plugins();

	done = 0;
	interrupt = 0;
	last_playback = 0;
	restart_plugins = 0;
	return 0;
}

void CommonRender::delete_vconsole()
{
	if(vconsole) delete vconsole;
	vconsole = 0;
}

int CommonRender::get_boundaries(int64_t &current_render_length)
{
	int64_t loop_end = tounits(renderengine->edl->local_session->loop_end, 1);
	int64_t loop_start = tounits(renderengine->edl->local_session->loop_start, 0);
	int64_t start_position = tounits(renderengine->command->start_position, 0);
	int64_t end_position = tounits(renderengine->command->end_position, 1);


// test absolute boundaries if no loop and not infinite
	if(renderengine->command->single_frame() || 
		(!renderengine->edl->local_session->loop_playback && 
		!renderengine->command->infinite))
	{
		if(renderengine->command->get_direction() == PLAY_FORWARD)
		{
			if(current_position + current_render_length >= end_position)
			{
				last_playback = 1;
				current_render_length = end_position - current_position;
			}
		}
// reverse playback
		else               
		{
			if(current_position - current_render_length <= start_position)
			{
				last_playback = 1;
				current_render_length = current_position - start_position;
			}
		}
	}

// test against loop boundaries
	if(!renderengine->command->single_frame() &&
		renderengine->edl->local_session->loop_playback && 
		!renderengine->command->infinite)
	{
		if(renderengine->command->get_direction() == PLAY_FORWARD)
		{
			int64_t segment_end = current_position + current_render_length;
			if(segment_end > loop_end)
			{
				current_render_length = loop_end - current_position;
			}
		}
		else
		{
			int64_t segment_end = current_position - current_render_length;
			if(segment_end < loop_start)
			{
				current_render_length = current_position - loop_start;
			}
		}
	}

	if(renderengine->command->single_frame())
		current_render_length = 1;

	if(current_render_length < 0) current_render_length = 0;
	return 0;
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
	current_position = 0;
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





int CommonRender::advance_position(int64_t current_render_length)
{
	int64_t loop_end = tounits(renderengine->edl->local_session->loop_end, 1);
	int64_t loop_start = tounits(renderengine->edl->local_session->loop_start, 0);

// advance the playback position
	if(renderengine->command->get_direction() == PLAY_REVERSE)
		current_position -= current_render_length;
	else
		current_position += current_render_length;

// test loop again
	if(renderengine->edl->local_session->loop_playback && 
		!renderengine->command->infinite)
	{
		if(renderengine->command->get_direction() == PLAY_REVERSE)
		{
			if(current_position <= loop_start)
				current_position = loop_end;
		}
		else
		{
			if(current_position >= loop_end)
				current_position = loop_start + (current_position - loop_end);
		}
	}
	return 0;
}

int64_t CommonRender::tounits(double position, int round)
{
	return (int64_t)position;
}

double CommonRender::fromunits(int64_t position)
{
	return (double)position;
}

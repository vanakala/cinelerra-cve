#include "auto.h"
#include "cache.h"
#include "commonrender.h"
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
 : Thread()
{
	this->renderengine = renderengine;
	reset_parameters();
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
//printf("CommonRender::~CommonRender 1\n");
}

void CommonRender::reset_parameters()
{
	total_modules = 0;
	set_synchronous(1);
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
	long temp_length = 1;

//printf("CommonRender::arm_command 1 %f\n", renderengine->command->playbackstart);
	current_position = tounits(renderengine->command->playbackstart, 0);
//printf("CommonRender::arm_command 2 %d\n", renderengine->command->playbackstart);

	init_output_buffers();

//printf("CommonRender::arm_command 3 %d\n", renderengine->command->playbackstart);
	last_playback = 0;
	if(test_reconfigure(current_position, temp_length))
	{
//printf("CommonRender::arm_command 3.1 %d\n", renderengine->command->playbackstart);
		restart_playback();
	}
	else
	{
		vconsole->start_playback();
	}
//printf("CommonRender::arm_command 4 %d\n", renderengine->command->playbackstart);

	done = 0;
	interrupt = 0;
	last_playback = 0;
	restart_plugins = 0;

//printf("CommonRender::arm_command 5\n");
}



void CommonRender::create_modules()
{
//printf("CommonRender::create_modules 1\n");
// Create a module for every track, playable or not
	Track *current = renderengine->edl->tracks->first;
	int module = 0;

//printf("CommonRender::create_modules 1\n");
	if(!modules)
	{
		total_modules = get_total_tracks();
		modules = new Module*[total_modules];
//printf("CommonRender::create_modules 1 %d\n", total_modules);

		for(module = 0; module < total_modules && current; current = NEXT)
		{
//printf("CommonRender::create_modules 2 %d %p\n", total_modules, current);
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
//printf("CommonRender::create_modules 3 %d\n", total_modules);
		for(module = 0; module < total_modules; module++)
		{
			modules[module]->create_objects();
		}
//printf("CommonRender::create_modules 4\n");
	}
}

void CommonRender::start_plugins()
{
//printf("CommonRender::start_plugins 1\n");
// Only start if virtual console was created
	if(restart_plugins)
	{
		for(int i = 0; i < total_modules; i++)
		{
			modules[i]->render_init();
		}
	}
//printf("CommonRender::start_plugins 2\n");
}

int CommonRender::test_reconfigure(long position, long &length)
{
	if(!vconsole) return 1;
	if(!modules) return 1;
	
	return vconsole->test_reconfigure(position, length, last_playback);
}


void CommonRender::build_virtual_console()
{
// Create new virtual console object
//printf("CommonRender::build_virtual_console 1 %p\n", vconsole);
	if(!vconsole)
	{
		vconsole = new_vconsole_object();
//printf("CommonRender::build_virtual_console 2\n");
	}

// Create nodes
	vconsole->create_objects();
//printf("CommonRender::build_virtual_console 3\n");
}

void CommonRender::start_command()
{
	if(renderengine->command->realtime)
	{
		start_lock.lock();
		Thread::start();
		start_lock.lock();
		start_lock.unlock();
	}
}

int CommonRender::restart_playback()
{
//printf("CommonRender::restart_playback 1\n");
	delete_vconsole();
//printf("CommonRender::restart_playback 2\n");
	create_modules();
//printf("CommonRender::restart_playback 3\n");
	build_virtual_console();
//printf("CommonRender::restart_playback 4\n");
	vconsole->start_playback();
//printf("CommonRender::restart_playback 5\n");
	start_plugins();
//printf("CommonRender::restart_playback 6\n");

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

int CommonRender::get_boundaries(long &current_render_length)
{
	long loop_end = tounits(renderengine->edl->local_session->loop_end, 1);
	long loop_start = tounits(renderengine->edl->local_session->loop_start, 0);
	long start_position = tounits(renderengine->command->start_position, 0);
	long end_position = tounits(renderengine->command->end_position, 1);


//printf("CommonRender::get_boundaries 1 %d %d %d %d\n", 
//current_render_length, 
//current_position, 
//start_position,
//end_position);
// test absolute boundaries if no loop and not infinite
	if(renderengine->command->single_frame() || 
		(!renderengine->edl->local_session->loop_playback && 
		!renderengine->command->infinite))
	{
//printf("CommonRender::get_boundaries 1.1 %d\n", current_render_length);
		if(renderengine->command->get_direction() == PLAY_FORWARD)
		{
//printf("CommonRender::get_boundaries 1.2 %d\n", current_render_length);
			if(current_position + current_render_length >= end_position)
			{
				last_playback = 1;
				current_render_length = end_position - current_position;
//printf("CommonRender::get_boundaries 1.3 %d\n", current_render_length);
			}
		}
// reverse playback
		else               
		{
//printf("CommonRender::get_boundaries 1.4 %d\n", current_render_length);
			if(current_position - current_render_length <= start_position)
			{
				last_playback = 1;
				current_render_length = current_position - start_position;
//printf("CommonRender::get_boundaries 1.5 %d\n", current_render_length);
			}
		}
	}
//printf("CommonRender::get_boundaries 2 %d\n", current_render_length);

// test against loop boundaries
	if(!renderengine->command->single_frame() &&
		renderengine->edl->local_session->loop_playback && 
		!renderengine->command->infinite)
	{

		if(renderengine->command->get_direction() == PLAY_FORWARD)
		{
			long segment_end = current_position + current_render_length;
			if(segment_end > loop_end)
			{
				current_render_length = loop_end - current_position;
			}
		}
		else
		{
			long segment_end = current_position - current_render_length;
			if(segment_end < loop_start)
			{
				current_render_length = current_position - loop_start;
			}
		}
	}
	
//printf("CommonRender::get_boundaries 3 %d\n", current_render_length);
	if(renderengine->command->single_frame())
		current_render_length = 1;

//printf("CommonRender::get_boundaries 4 %d\n", current_render_length);
//sleep(1);
	return 0;
}

void CommonRender::run()
{
	start_lock.unlock();
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





int CommonRender::advance_position(long current_render_length)
{
	long loop_end = tounits(renderengine->edl->local_session->loop_end, 1);
	long loop_start = tounits(renderengine->edl->local_session->loop_start, 0);

// advance the playback position
	if(renderengine->command->get_direction() == PLAY_REVERSE)
		current_position -= current_render_length;
	else
		current_position += current_render_length;

//printf("CommonRender::advance_position 1 %d\n", current_render_length);
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

long int CommonRender::tounits(double position, int round)
{
	return (long)position;
}

double CommonRender::fromunits(long position)
{
	return (double)position;
}

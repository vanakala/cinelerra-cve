#include "auto.h"
#include "automation.h"
#include "autos.h"
#include "commonrender.h"
#include "edl.h"
#include "edlsession.h"
#include "virtualconsole.h"
#include "module.h"
#include "mutex.h"
#include "playabletracks.h"
#include "renderengine.h"
#include "intautos.h"
#include "tracks.h"
#include "transportque.h"
#include "virtualnode.h"


VirtualConsole::VirtualConsole(RenderEngine *renderengine, 
	CommonRender *commonrender,
	int data_type)
 : Thread()
{
	this->renderengine = renderengine;
	this->commonrender = commonrender;
	this->data_type = data_type;
	total_tracks = 0;
	startup_lock = new Mutex;
	playable_tracks = 0;
	ring_buffers = 0;
	virtual_modules = 0;
}


VirtualConsole::~VirtualConsole()
{
// Destructor always calls default methods so can't put deletions in here
	for(int i = 0; i < total_tracks; i++)
		delete virtual_modules[i];

	if(total_tracks) delete [] virtual_modules;

	delete startup_lock;
	if(playable_tracks) delete playable_tracks;
}

int VirtualConsole::total_ring_buffers()
{
	return 2;
}


void VirtualConsole::create_objects()
{
	interrupt = 0;
	done = 0;
	current_input_buffer = 0;
	current_vconsole_buffer = 0;

//printf("VirtualConsole::create_objects 1\n");
	get_playable_tracks();
//printf("VirtualConsole::create_objects 2 %p\n", playable_tracks);
	total_tracks = playable_tracks->total;
//printf("VirtualConsole::create_objects 3\n");
	allocate_input_buffers();
//printf("VirtualConsole::create_objects 4\n");
	build_virtual_console(1);
//printf("VirtualConsole::create_objects 5\n");
	sort_virtual_console();
//printf("VirtualConsole::create_objects 6\n");
//dump();
//printf("VirtualConsole::create_objects 7\n");
}

void VirtualConsole::start_playback()
{
	done = 0;
	interrupt = 0;
	current_input_buffer = 0;
	current_vconsole_buffer = 0;
//printf("VirtualConsole::start_playback 1 %d %d\n", renderengine->command->realtime, data_type);
	if(renderengine->command->realtime && data_type == TRACK_AUDIO)
	{
// don't start a thread unless writing to an audio device
		startup_lock->lock();
		for(int ring_buffer = 0; ring_buffer < ring_buffers; ring_buffer++)
		{
			input_lock[ring_buffer]->reset();
			output_lock[ring_buffer]->reset();
			input_lock[ring_buffer]->lock();
		}
		Thread::set_synchronous(1);   // prepare thread base class
//printf("VirtualConsole::start_playback 2 %d\n", renderengine->edl->session->real_time_playback);
		Thread::start();
//printf("VirtualConsole::start_playback 3 %d\n", renderengine->edl->session->real_time_playback);
		startup_lock->lock();
		startup_lock->unlock();
	}
}


Module* VirtualConsole::module_of(Track *track)
{
	for(int i = 0; i < commonrender->total_modules; i++)
	{
//printf("VirtualConsole::module_of %p %p\n", (Track*)commonrender->modules[i]->track, track); 
		if(commonrender->modules[i]->track == track) return commonrender->modules[i];
	}
	return 0;
}

Module* VirtualConsole::module_number(int track_number)
{
// The track number is an absolute number of the track independant of
// the tracks with matching data type but virtual modules only exist for
// the matching data type.
// Convert from absolute track number to data type track number.
	Track *current = renderengine->edl->tracks->first;
	int data_type_number = 0, number = 0;

	for( ; current; current = NEXT, number++)
	{
		if(current->data_type == data_type)
		{
			if(number == track_number)
				return commonrender->modules[data_type_number];
			else
				data_type_number++;
		}
	}


	return 0;
}

int VirtualConsole::allocate_input_buffers()
{
	if(!ring_buffers)
	{
		ring_buffers = total_ring_buffers();

// allocate the drive read buffers
		for(int ring_buffer = 0; 
			ring_buffer < ring_buffers; 
			ring_buffer++)
		{
			input_lock[ring_buffer] = new Mutex;
			output_lock[ring_buffer] = new Mutex;
			last_playback[ring_buffer] = 0;
			new_input_buffer(ring_buffer);
		}
	}

	return 0;
}

void VirtualConsole::build_virtual_console(int persistant_plugins)
{
// allocate the virtual modules
//printf("VirtualConsole::build_virtual_console 1\n");
	if(!virtual_modules)
	{
		virtual_modules = new VirtualNode*[total_tracks];

//printf("VirtualConsole::build_virtual_console 2 %d %d\n", data_type, total_tracks);
		for(int i = 0; i < total_tracks; i++)
		{
//printf("VirtualConsole::build_virtual_console 3\n");
			virtual_modules[i] = new_toplevel_node(playable_tracks->values[i], 
				module_of(playable_tracks->values[i]), 
				i);

// Expand the track
			virtual_modules[i]->expand(persistant_plugins, commonrender->current_position);
//printf("VirtualConsole::build_virtual_console 3\n");
		}
		commonrender->restart_plugins = 1;
	}

}

int VirtualConsole::sort_virtual_console()
{
// sort the console
	int done = 0, result = 0;
	long attempts = 0;
	int i;

//printf("VirtualConsole::sort_virtual_console 1\n");
	if(!render_list.total)
	{
		while(!done && attempts < 50)
		{
// Sort iteratively until all the remaining plugins can be rendered.
// Iterate backwards so video is composited properly
			done = 1;
			for(i = total_tracks - 1; i >= 0; i--)
			{
				result = virtual_modules[i]->sort(&render_list);
				if(result) done = 0;
			}
			attempts++;
		}

//printf("VirtualConsole::sort_virtual_console 2 %d\n", render_list.total);
// prevent short circuts
		if(attempts >= 50)
		{
			printf("VirtualConsole::sort_virtual_console: Recursive.\n");
		}
//printf("VirtualConsole::sort_virtual_console 2\n");
	}
	return 0;
}

void VirtualConsole::dump()
{
	printf("VirtualConsole\n");
	printf(" Modules\n");
	for(int i = 0; i < commonrender->total_modules; i++)
		commonrender->modules[i]->dump();
	printf(" Nodes\n");
	for(int i = 0; i < total_tracks; i++)
		virtual_modules[i]->dump(0);
}


int VirtualConsole::test_reconfigure(long position, 
	long &length, 
	int &last_playback)
{
	int result = 0;
	Track *current_track;
	Module *module;

// printf("VirtualConsole::test_reconfigure 2 %d %p %p\n", 
// renderengine->config->vconfig->do_channel[0],
// renderengine->config->vconfig->do_channel,
// playable_tracks->do_channel);

//printf("VirtualConsole::test_reconfigure 1 %d\n", playable_tracks->total);

// Test playback status against virtual console for current position.
	for(current_track = renderengine->edl->tracks->first;
		current_track && !result;
		current_track = current_track->next)
	{
		if(current_track->data_type == data_type)
		{
// Playable status changed
// printf("VirtualConsole::test_reconfigure 2 %d\n", 
// current_track->data_type);
			if(playable_tracks->is_playable(current_track, 
				commonrender->current_position))
			{
				if(!playable_tracks->is_listed(current_track))
					result = 1;
			}
			else
			if(playable_tracks->is_listed(current_track))
			{
				result = 1;
//printf("VirtualConsole::test_reconfigure 3 %d\n", result);
			}
		}
	}

//printf("VirtualConsole::test_reconfigure 2 %d %d\n", length, result);
// Test plugins against virtual console at current position
	for(int i = 0; i < commonrender->total_modules && !result; i++)
		result = commonrender->modules[i]->test_plugins();

//printf("VirtualConsole::test_reconfigure 3 %d %d\n", length, result);




// Now get the length of time until next reconfiguration.
// This part is not concerned with result.
// Don't clip input length if only rendering 1 frame.
	if(length == 1)  return result;





	int direction = renderengine->command->get_direction();
	long longest_duration;

//printf("VirtualConsole::test_reconfigure 6 %d %d\n", length, result);
// Length of time until next transition, edit, or effect change.
// Why do we need the edit change?  Probably for changing to and from silence.
	for(current_track = renderengine->edl->tracks->first;
		current_track;
		current_track = current_track->next)
	{
//printf("VirtualConsole::test_reconfigure 7 %d %d\n", result, length);
		if(current_track->data_type == data_type)
		{
//printf("VirtualConsole::test_reconfigure 8 %d %d\n", result, length);
			longest_duration = current_track->edit_change_duration(commonrender->current_position, length, direction, 1);

			if(longest_duration < length)
			{
				length = longest_duration;
				last_playback = 0;
			}
//printf("VirtualConsole::test_reconfigure 9 %d %d\n", result, length);

			if(renderengine->edl->session->test_playback_edits)
			{
				longest_duration = current_track->edit_change_duration(commonrender->current_position, length, direction, 0);
				if(longest_duration < length)
				{
					length = longest_duration;
					last_playback = 0;
				}
			}
//printf("VirtualConsole::test_reconfigure 10 %d %d\n", result, length);
		}
	}
//printf("VirtualConsole::test_reconfigure 11 %d %d\n", length, result);

	return result;
}

void VirtualConsole::run()
{
	startup_lock->unlock();
}






















int VirtualConsole::delete_virtual_console()
{
// delete the virtual modules
	for(int i = 0; i < total_tracks; i++)
	{
		delete virtual_modules[i];
	}
	delete [] virtual_modules;

// delete sort order
	render_list.remove_all();
}

int VirtualConsole::delete_input_buffers()
{
// delete input buffers
	for(int buffer = 0; buffer < total_ring_buffers(); buffer++)
	{
		delete_input_buffer(buffer);
	}

	for(int i = 0; i < total_ring_buffers(); i++)
	{
		delete input_lock[i];
		delete output_lock[i];
	}

	delete playable_tracks;
	total_tracks = 0;
	return 0;
}

int VirtualConsole::start_rendering(int duplicate)
{
	this->interrupt = 0;

	if(renderengine->command->realtime && commonrender->asynchronous)
	{
// don't start a thread unless writing to an audio device
		startup_lock->lock();
		set_synchronous(1);   // prepare thread base class
		start();
	}
	return 0;
}

int VirtualConsole::wait_for_completion()
{
	if(renderengine->command->realtime && commonrender->asynchronous)
	{
		join();
	}
	return 0;
}

int VirtualConsole::swap_input_buffer()
{
	current_input_buffer++;
	if(current_input_buffer >= total_ring_buffers()) current_input_buffer = 0;
	return 0;
}

int VirtualConsole::swap_thread_buffer()
{
	current_vconsole_buffer++;
	if(current_vconsole_buffer >= total_ring_buffers()) current_vconsole_buffer = 0;
	return 0;
}


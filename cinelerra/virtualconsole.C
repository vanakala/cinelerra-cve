#include "auto.h"
#include "automation.h"
#include "autos.h"
#include "commonrender.h"
#include "condition.h"
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
	startup_lock = new Condition(1, "VirtualConsole::startup_lock");
	playable_tracks = 0;
	ring_buffers = 0;
	virtual_modules = 0;
}


VirtualConsole::~VirtualConsole()
{
//printf("VirtualConsole::~VirtualConsole 1\n");
	delete_virtual_console();
	delete_input_buffers();

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

	get_playable_tracks();
	total_tracks = playable_tracks->total;
	allocate_input_buffers();
	build_virtual_console(1);
	sort_virtual_console();
//dump();

}

void VirtualConsole::start_playback()
{
	done = 0;
	interrupt = 0;
	current_input_buffer = 0;
	current_vconsole_buffer = 0;
	if(renderengine->command->realtime && data_type == TRACK_AUDIO)
	{
// don't start a thread unless writing to an audio device
		startup_lock->lock();
		for(int ring_buffer = 0; ring_buffer < ring_buffers; ring_buffer++)
		{
			input_lock[ring_buffer]->reset();
			output_lock[ring_buffer]->reset();
			input_lock[ring_buffer]->lock("VirtualConsole::start_playback");
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
			input_lock[ring_buffer] = new Condition(1, "VirtualConsole::input_lock");
			output_lock[ring_buffer] = new Condition(1, "VirtualConsole::output_lock");
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
	int64_t attempts = 0;
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


int VirtualConsole::test_reconfigure(int64_t position, 
	int64_t &length, 
	int &last_playback)
{
	int result = 0;
	Track *current_track;
	Module *module;

// Test playback status against virtual console for current position.
	for(current_track = renderengine->edl->tracks->first;
		current_track && !result;
		current_track = current_track->next)
	{
		if(current_track->data_type == data_type)
		{
// Playable status changed
			if(playable_tracks->is_playable(current_track, 
				commonrender->current_position,
				1))
			{
				if(!playable_tracks->is_listed(current_track))
					result = 1;
			}
			else
			if(playable_tracks->is_listed(current_track))
			{
				result = 1;
			}
		}
	}

// Test plugins against virtual console at current position
	for(int i = 0; i < commonrender->total_modules && !result; i++)
		result = commonrender->modules[i]->test_plugins();





// Now get the length of time until next reconfiguration.
// This part is not concerned with result.
// Don't clip input length if only rendering 1 frame.
	if(length == 1) return result;





	int direction = renderengine->command->get_direction();
// GCC 3.2 requires this or optimization error results.
	int64_t longest_duration1;
	int64_t longest_duration2;
	int64_t longest_duration3;

// Length of time until next transition, edit, or effect change.
// Why do we need the edit change?  Probably for changing to and from silence.
	for(current_track = renderengine->edl->tracks->first;
		current_track;
		current_track = current_track->next)
	{
		if(current_track->data_type == data_type)
		{
// Test the transitions
			longest_duration1 = current_track->edit_change_duration(
				commonrender->current_position, 
				length, 
				direction == PLAY_REVERSE, 
				1,
				1);


// Test the edits
			longest_duration2 = current_track->edit_change_duration(
				commonrender->current_position, 
				length, 
				direction, 
				0,
				1);


// Test the plugins
			longest_duration3 = current_track->plugin_change_duration(
				commonrender->current_position,
				length,
				direction == PLAY_REVERSE,
				1);

			if(longest_duration1 < length)
			{
				length = longest_duration1;
				last_playback = 0;
			}
			if(longest_duration2 < length)
			{
				length = longest_duration2;
				last_playback = 0;
			}
			if(longest_duration3 < length)
			{
				length = longest_duration3;
				last_playback = 0;
			}

		}
	}

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
// Seems to get allocated even if new[0].
	if(virtual_modules) delete [] virtual_modules;
	virtual_modules = 0;

// delete sort order
	render_list.remove_all();
}

int VirtualConsole::delete_input_buffers()
{
// delete input buffers
	for(int buffer = 0; buffer < ring_buffers; buffer++)
	{
		delete_input_buffer(buffer);
	}

	for(int i = 0; i < ring_buffers; i++)
	{
		delete input_lock[i];
		delete output_lock[i];
	}

	total_tracks = 0;
	ring_buffers = 0;
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


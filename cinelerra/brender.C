
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

#include "asset.h"
#include "bcsignals.h"
#include "brender.h"
#include "clip.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "mainsession.h"
#include "mtimebar.h"
#include "mutex.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "packagedispatcher.h"
#include "preferences.h"
#include "renderfarm.h"
#include "tracks.h"
#include "units.h"


#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>



extern "C"
{
#include <uuid/uuid.h>
}






BRender::BRender(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
	map_lock = new Mutex("BRender::map_lock");
	completion_lock = new Condition(0, "BRender::completion_lock");
	timer = new Timer;
	socket_path[0] = 0;
	thread = 0;
	master_pid = -1;
	arguments[0] = arguments[1] = arguments[2] = 0;
	map = 0;
	map_size = 0;
	map_valid = 0;
	last_contiguous + 0;
	set_synchronous(1);
}

BRender::~BRender()
{
TRACE("BRender::~BRender 1\n");
	if(thread) 
	{
TRACE("BRender::~BRender 2\n");
		stop();
TRACE("BRender::~BRender 3\n");
		delete thread;
TRACE("BRender::~BRender 4\n");
	}


TRACE("BRender::~BRender 5\n");
	if(master_pid >= 0)
	{
		kill(master_pid, SIGKILL);
TRACE("BRender::~BRender 6\n");
		Thread::join();
TRACE("BRender::~BRender 7\n");
	}

TRACE("BRender::~BRender 8\n");
	delete map_lock;
TRACE("BRender::~BRender 9\n");
	delete completion_lock;
TRACE("BRender::~BRender 10\n");
UNSET_TEMP(socket_path);
	remove(socket_path);
TRACE("BRender::~BRender 11\n");
	if(arguments[0]) delete [] arguments[0];
TRACE("BRender::~BRender 12\n");
	if(arguments[1]) delete [] arguments[1];
TRACE("BRender::~BRender 13\n");
	if(arguments[2]) delete [] arguments[2];
TRACE("BRender::~BRender 14\n");
	if(map) delete [] map;
TRACE("BRender::~BRender 15\n");
	delete timer;
TRACE("BRender::~BRender 100\n");
}

void BRender::initialize()
{
	timer->update();
// Create socket for background process.
	uuid_t socket_temp;
	sprintf(socket_path, "/tmp/cinelerra.");
	uuid_generate(socket_temp);
	uuid_unparse(socket_temp, socket_path + strlen(socket_path));
SET_TEMP(socket_path);

// Start background instance of executable since codecs aren't reentrant
	Thread::start();

// Wait for local node to start
	thread = new BRenderThread(mwindow, this);
	thread->initialize();
}

void BRender::run()
{
	char string[BCTEXTLEN];
	int size;
	FILE *fd;
//printf("BRender::run 1 %d\n", getpid());


// Construct executable command with the designated filesystem port
	fd = fopen("/proc/self/cmdline", "r");
	if(fd)
	{
		fread(string, 1, BCTEXTLEN, fd);
		fclose(fd);
	}
	else
		perror(_("BRender::fork_background: can't open /proc/self/cmdline.\n"));

	arguments[0] = new char[strlen(string) + 1];
	strcpy(arguments[0], string);

	strcpy(string, "-b");
	arguments[1] = new char[strlen(string) + 1];
	strcpy(arguments[1], string);

	arguments[2] = new char[strlen(socket_path) + 1];
	strcpy(arguments[2], socket_path);
//printf("BRender::fork_background 1 %s\n", socket_path);

	arguments[3] = 0;

	int pid = vfork();
	if(!pid)
	{
		execvp(arguments[0], arguments);
		perror("BRender::fork_background");
		_exit(0);
	}

	master_pid = pid;
//printf("BRender::fork_background 1 %d\n", master_pid);



	int return_value;
	if(waitpid(master_pid, &return_value, WUNTRACED) < 0)
	{
		perror("BRender::run waitpid");
	}
}

// Give the last position of the EDL which hasn't changed.
// We copy the EDL and restart rendering at the lesser of position and
// our position.
void BRender::restart(EDL *edl)
{
//printf("BRender::restart 1\n");
	BRenderCommand *new_command = new BRenderCommand;
	map_valid = 0;
	new_command->copy_edl(edl);
	new_command->command = BRenderCommand::BRENDER_RESTART;
//printf("BRender::restart 2\n");
	thread->send_command(new_command);
//printf("BRender::restart 3\n");
// Map should be reallocated before this returns.
}

void BRender::stop()
{
//printf("BRender::stop 1\n");
	BRenderCommand *new_command = new BRenderCommand;
//printf("BRender::stop 1\n");
	new_command->command = BRenderCommand::BRENDER_STOP;
//printf("BRender::stop 1\n");
	thread->send_command(new_command);
//printf("BRender::stop 1\n");
	completion_lock->lock("BRender::stop");
//printf("BRender::stop 2\n");
}



int BRender::get_last_contiguous(int64_t brender_start)
{
	int result;
	map_lock->lock("BRender::get_last_contiguous");
	if(map_valid)
		result = last_contiguous;
	else
		result = brender_start;
	map_lock->unlock();
	return result;
}

void BRender::allocate_map(int64_t brender_start, int64_t start, int64_t end)
{
	map_lock->lock("BRender::allocate_map");
	unsigned char *old_map = map;
	map = new unsigned char[end];
	if(old_map)
	{
		memcpy(map, old_map, start);
		delete [] old_map;
	}

// Zero all before brender start
	bzero(map, brender_start);
// Zero all after current start
	bzero(map + start, end - start);

	map_size = end;
	map_valid = 1;
	last_contiguous = start;
	mwindow->session->brender_end = (double)last_contiguous / 
		mwindow->edl->session->frame_rate;
	map_lock->unlock();
}

int BRender::set_video_map(int64_t position, int value)
{
	int update_gui = 0;
	map_lock->lock("BRender::set_video_map");


	if(value == BRender::NOT_SCANNED)
	{
		printf(_("BRender::set_video_map called to set NOT_SCANNED\n"));
	}

// Preroll
	if(position < 0)
	{
		;
	}
	else
// In range
	if(position < map_size)
	{
		map[position] = value;
	}
	else
// Obsolete EDL
	{
		printf(_("BRender::set_video_map %d: attempt to set beyond end of map %d.\n"),
			position,
			map_size);
	}

// Maintain last contiguous here to reduce search time
	if(position == last_contiguous)
	{
		int i;
		for(i = position + 1; i < map_size && map[i]; i++)
		{
			;
		}
		last_contiguous = i;
		mwindow->session->brender_end = (double)last_contiguous / 
			mwindow->edl->session->frame_rate;

		if(timer->get_difference() > 1000 || last_contiguous >= map_size)
		{
			update_gui = 1;
			timer->update();
		}
	}

	map_lock->unlock();

	if(update_gui)
	{
		mwindow->gui->lock_window("BRender::set_video_map");
		mwindow->gui->timebar->update(1, 0);
		mwindow->gui->timebar->flush();
		mwindow->gui->unlock_window();
	}
	return 0;
}














BRenderCommand::BRenderCommand()
{
	edl = 0;
	command = BRENDER_NONE;
	position = 0.0;
}

BRenderCommand::~BRenderCommand()
{
// EDL should be zeroed if copied
	if(edl) delete edl;
}

void BRenderCommand::copy_from(BRenderCommand *src)
{
	this->edl = src->edl;
	src->edl = 0;
	this->position = src->position;
	this->command = src->command;
}


void BRenderCommand::copy_edl(EDL *edl)
{
	this->edl = new EDL;
	this->edl->create_objects();
	this->edl->copy_all(edl);
	this->position = 0;
}












BRenderThread::BRenderThread(MWindow *mwindow, BRender *brender)
 : Thread(1)
{
	this->mwindow = mwindow;
	this->brender = brender;
	input_lock = new Condition(0, "BRenderThread::input_lock");
	thread_lock = new Mutex("BRenderThread::thread_lock");
	total_frames_lock = new Mutex("BRenderThread::total_frames_lock");
	command_queue = 0;
	command = 0;
	done = 0;
	farm_server = 0;
	farm_result = 0;
	preferences = 0;
}

BRenderThread::~BRenderThread()
{
	thread_lock->lock("BRenderThread::~BRenderThread");
	done = 1;
	input_lock->unlock();
	thread_lock->unlock();
	Thread::join();
	delete input_lock;
	delete thread_lock;
	delete total_frames_lock;
	if(command) delete command;
	if(command_queue) delete command_queue;
	if(preferences) delete preferences;
}


void BRenderThread::initialize()
{
	Thread::start();
}

void BRenderThread::send_command(BRenderCommand *command)
{
TRACE("BRenderThread::send_command 1");
	thread_lock->lock("BRenderThread::send_command");
TRACE("BRenderThread::send_command 10");

	if(this->command_queue)
	{
		delete this->command_queue;
		this->command_queue = 0;
	}
	this->command_queue = command;
TRACE("BRenderThread::send_command 20");


	input_lock->unlock();
	thread_lock->unlock();
}

int BRenderThread::is_done(int do_lock)
{
	if(do_lock) thread_lock->lock("BRenderThread::is_done");
	int result = done;
	if(do_lock) thread_lock->unlock();
	return result;
}

void BRenderThread::run()
{
	while(!is_done(1))
	{
		BRenderCommand *new_command = 0;
		thread_lock->lock("BRenderThread::run 1");

// Got new command
		if(command_queue)
		{
			;
		}
		else
// Wait for new command
		{
			thread_lock->unlock();
			input_lock->lock("BRenderThread::run 2");
			thread_lock->lock("BRenderThread::run 3");
		}

// Pull the command off
		if(!is_done(0))
		{
			new_command = command_queue;
			command_queue = 0;
		}

		thread_lock->unlock();




// Process the command here to avoid delay.
// Quit condition
		if(!new_command)
		{
			;
		}
		else
		if(new_command->command == BRenderCommand::BRENDER_STOP)
		{
			stop();
			delete new_command;
			new_command = 0;
//			if(command) delete command;
//			command = new_command;
		}
		else
		if(new_command->command == BRenderCommand::BRENDER_RESTART)
		{
// Compare EDL's and get last equivalent position in new EDL
			if(command && command->edl)
				new_command->position = 
					new_command->edl->equivalent_output(command->edl);
			else
				new_command->position = 0;


			stop();
//printf("BRenderThread::run 4\n");
			brender->completion_lock->lock("BRenderThread::run 4");
//printf("BRenderThread::run 5\n");

			if(new_command->edl->tracks->total_playable_vtracks())
			{
				if(command) delete command;
				command = new_command;
//printf("BRenderThread::run 6\n");
				start();
//printf("BRenderThread::run 7\n");
			}
			else
			{
//printf("BRenderThread::run 8 %p\n", farm_server);
				delete new_command;
				new_command = 0;
			}
		}
	}
}

void BRenderThread::stop()
{
	if(farm_server)
	{
		farm_result = 1;
		farm_server->wait_clients();
		delete farm_server;
		delete packages;
		delete preferences;
		farm_server = 0;
		packages = 0;
		preferences = 0;
	}
	brender->completion_lock->unlock();
}

void BRenderThread::start()
{
// Reset return parameters
	farm_result = 0;
	fps_result = 0;
	total_frames = 0;
	int result = 0;

// Allocate render farm.
	if(!farm_server)
	{
//printf("BRenderThread::start 1\n");
		preferences = new Preferences;
		preferences->copy_from(mwindow->preferences);
		packages = new PackageDispatcher;

// Fix preferences to use local node
		if(!preferences->use_renderfarm)
		{
			preferences->use_renderfarm = 1;
			preferences->delete_nodes();
		}
		preferences->add_node(brender->socket_path,
			0,
			1,
			preferences->local_rate);
//printf("BRenderThread::start 1 %s\n", brender->socket_path);
		preferences->brender_asset->use_header = 0;
		preferences->brender_asset->frame_rate = command->edl->session->frame_rate;
		preferences->brender_asset->width = command->edl->session->output_w;
		preferences->brender_asset->height = command->edl->session->output_h;
		preferences->brender_asset->interlace_mode = command->edl->session->interlace_mode;

// Get last contiguous and reset map.
// If the framerate changes, last good should be 0 from the user.
		int brender_start = (int)(command->edl->session->brender_start *
			command->edl->session->frame_rate);
		int last_contiguous = brender->last_contiguous;
		int last_good = (int)(command->edl->session->frame_rate * 
			command->position);
		if(last_good < 0) last_good = last_contiguous;
		int start_frame = MIN(last_contiguous, last_good);
		start_frame = MAX(start_frame, brender_start);
		int64_t end_frame = Units::round(command->edl->tracks->total_video_length() * 
			command->edl->session->frame_rate);
		if(end_frame < start_frame) end_frame = start_frame;


printf("BRenderThread::start 1 map=%d equivalent=%d brender_start=%d result=%d end=%d\n", 
last_contiguous, 
last_good, 
brender_start, 
start_frame,
end_frame);

//sleep(1);

		brender->allocate_map(brender_start, start_frame, end_frame);
//sleep(1);
//printf("BRenderThread::start 2\n");

		result = packages->create_packages(mwindow,
			command->edl,
			preferences,
			BRENDER_FARM, 
			preferences->brender_asset, 
			(double)start_frame / command->edl->session->frame_rate, 
			(double)end_frame / command->edl->session->frame_rate,
			0);

//sleep(1);
//printf("BRenderThread::start 3 %d\n", result);
		farm_server = new RenderFarmServer(mwindow->plugindb, 
			packages,
			preferences,
			0,
			&farm_result,
			&total_frames,
			total_frames_lock,
			preferences->brender_asset,
			command->edl,
			brender);

//sleep(1);
//printf("BRenderThread::start 4\n");
		result = farm_server->start_clients();

//sleep(1);
// No local rendering because of codec problems.


// Abort
		if(result)
		{
// No-one must be retrieving a package when packages are deleted.
//printf("BRenderThread::start 7 %p\n", farm_server);
			delete farm_server;
			delete packages;
//printf("BRenderThread::start 8 %p\n", preferences);
			delete preferences;
//printf("BRenderThread::start 9\n");
			farm_server = 0;
			packages = 0;
			preferences = 0;
		}
//sleep(1);
//printf("BRenderThread::start 10\n");

	}
}



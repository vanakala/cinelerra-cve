// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "bcsignals.h"
#include "bctimer.h"
#include "brender.h"
#include "clip.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "mainsession.h"
#include "mainerror.h"
#include "mutex.h"
#include "mwindow.h"
#include "packagedispatcher.h"
#include "preferences.h"
#include "renderfarm.h"
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
 : Thread(THREAD_SYNCHRONOUS)
{
	this->mwindow = mwindow;
	map_lock = new Mutex("BRender::map_lock");
	completion_lock = new Condition(0, "BRender::completion_lock");
	timer = new Timer;
	socket_path[0] = 0;
	thread = 0;
	master_pid = -1;
	arguments[0] = arguments[1] = arguments[2] = 0;
	map_valid = 0;
}

BRender::~BRender()
{
	if(thread) 
	{
		stop();
		delete thread;
	}

	if(master_pid >= 0)
	{
		kill(master_pid, SIGKILL);
		Thread::join();
	}

	delete map_lock;
	delete completion_lock;
	UNSET_TEMP(socket_path);
	remove(socket_path);
	if(arguments[0]) delete [] arguments[0];
	if(arguments[1]) delete [] arguments[1];
	if(arguments[2]) delete [] arguments[2];
	delete timer;
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

// Construct executable command with the designated filesystem port
	fd = fopen("/proc/self/cmdline", "r");
	if(fd)
	{
		if(fread(string, 1, BCTEXTLEN, fd) <= 0)
			perror(_("BRender::fork_background: can't read /proc/self/cmdline.\n"));
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

	arguments[3] = 0;

	int pid = vfork();
	if(!pid)
	{
		execvp(arguments[0], arguments);
		perror("BRender::fork_background");
		_exit(0);
	}

	master_pid = pid;

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
	BRenderCommand *new_command = new BRenderCommand;
	map_valid = 0;
	new_command->edl = edl;
	new_command->command = BRenderCommand::BRENDER_RESTART;
	thread->send_command(new_command);
// Map should be reallocated before this returns.
}

void BRender::stop()
{
	BRenderCommand *new_command = new BRenderCommand;
	new_command->command = BRenderCommand::BRENDER_STOP;
	thread->send_command(new_command);
	completion_lock->lock("BRender::stop");
}

void BRender::render_done()
{
	if(thread->farm_result)
	{
		// brender failed
		videomap.set_map(0.0, videomap.last->pts, 0);
		edlsession->brender_start = 0;
		mainsession->brender_end = 0;
		errormsg(_("Background rendering failed"));
	}
	mwindow->update_gui(WUPD_TIMEBAR);
}

void BRender::allocate_map(ptstime brender_start, ptstime start, ptstime end)
{
	map_lock->lock("BRender::allocate_map");

	videomap.set_map(0, brender_start, 0);
	videomap.set_map(start, end, 0);
	map_valid = 1;
	mainsession->brender_end = start;
	map_lock->unlock();
}

void BRender::set_video_map(ptstime start, ptstime end)
{
	int update_gui = 0;
	map_lock->lock("BRender::set_video_map");

	if(start < 0)
		start = 0;
	if(end < 0)
		end = 0;
	if(PTSEQU(start, end))
		return;
	videomap.set_map(start, end, 1);

// Maintain last contiguous here to reduce search time
	if(videomap.last->pts > mainsession->brender_end)
	{
		mainsession->brender_end = videomap.last->pts;

		if(timer->get_difference() > 1000)
		{
			update_gui = 1;
			timer->update();
		}
	}

	map_lock->unlock();

	if(update_gui)
		mwindow->update_gui(WUPD_TIMEBAR);
}


BRenderCommand::BRenderCommand()
{
	edl = 0;
	position = 0;
	command = BRENDER_NONE;
}


BRenderThread::BRenderThread(MWindow *mwindow, BRender *brender)
 : Thread(THREAD_SYNCHRONOUS)
{
	this->mwindow = mwindow;
	this->brender = brender;
	input_lock = new Condition(0, "BRenderThread::input_lock");
	thread_lock = new Mutex("BRenderThread::thread_lock");
	command_queue = 0;
	command = 0;
	done = 0;
	farm_server = 0;
	farm_result = 0;
	preferences = 0;
	edl = 0;
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
	if(command) delete command;
	if(command_queue) delete command_queue;
	if(preferences) delete preferences;
	delete edl;
}

void BRenderThread::initialize()
{
	Thread::start();
}

void BRenderThread::send_command(BRenderCommand *command)
{
	thread_lock->lock("BRenderThread::send_command");

	if(this->command_queue)
	{
		delete this->command_queue;
		this->command_queue = 0;
	}
	this->command_queue = command;

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
		}
		else
		if(new_command->command == BRenderCommand::BRENDER_RESTART)
		{
// Compare EDL's and get last equivalent position in new EDL
			if(command && edl)
				new_command->position = 
					new_command->edl->equivalent_output(edl);
			else
				new_command->position = 0;

			stop();
			brender->completion_lock->lock("BRenderThread::run 4");

			if(new_command->edl->playable_tracks_of(TRACK_VIDEO))
			{
				if(command) delete command;
				command = new_command;
				delete edl;
				edl = new EDL(0);
				edl->copy_all(command->edl);
				start();
			}
			else
			{
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
		farm_result = 0;
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
	int result = 0;

// Allocate render farm.
	if(!farm_server)
	{
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
		preferences->brender_asset->use_header = 0;
		preferences->brender_asset->frame_rate = edlsession->frame_rate;
		preferences->brender_asset->width = edlsession->output_w;
		preferences->brender_asset->height = edlsession->output_h;
		preferences->brender_asset->interlace_mode = edlsession->interlace_mode;
		preferences->brender_asset->sample_aspect_ratio = edlsession->sample_aspect_ratio;
		preferences->brender_asset->video_data = 1;
		preferences->brender_asset->init_streams();

// Get last contiguous and reset map.
// If the framerate changes, last good should be 0 from the user.
		ptstime brender_start = edlsession->brender_start;
		ptstime last_contiguous = brender->videomap.last->pts;
		ptstime last_good = command->position;
		if(last_good < 0) last_good = last_contiguous;
		ptstime start_pts = MIN(last_contiguous, last_good);
		start_pts = MAX(start_pts, brender_start);
		ptstime end_pts = command->edl->total_length_of(TRACK_VIDEO);
		if(end_pts < start_pts) end_pts = start_pts;
		brender->allocate_map(brender_start, start_pts, end_pts);
		result = packages->create_packages(mwindow,
			command->edl,
			preferences,
			RENDER_BRENDER,
			preferences->brender_asset, 
			start_pts,
			end_pts,
			0);

		farm_server = new RenderFarmServer(packages,
			preferences,
			0,
			&farm_result,
			0,
			0,
			preferences->brender_asset,
			command->edl,
			brender);
		result = farm_server->start_clients();
// No local rendering because of codec problems.

// Abort
		if(result)
		{
// No-one must be retrieving a package when packages are deleted.
			delete farm_server;
			delete packages;
			delete preferences;
			farm_server = 0;
			packages = 0;
			preferences = 0;
		}
	}
}

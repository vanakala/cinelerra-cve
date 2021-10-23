// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BRENDER_H
#define BRENDER_H

// The master node of the background renderer needs a separate memory space 
// because few of the codecs are reentrant.

// To solve the problem, the master node forks itself and treats the forked 
// master node as the first node of a renderfarm.  There is no real master node
// of the renderfarm.  The BRender object is a thread in order to
// join the forked master node.

// If renderfarm is enabled, the extra renderfarm nodes are treated normally.
// Unfortunately because of the codec problem, only one copy of Cinelerra
// can be running on a single renderfarm.  This means either background 
// rendering or foreground rendering can be happening but not both.

// A BRenderThread client runs in the background and a BRender object 
// interfaces the main window.  The BRender client recieves commands to
// restart, start, and stop background rendering on its own time to avoid 
// interrupting the main window.

// Whenever a change happens to the timeline, we calculate the last position
// which hasn't changed and the end of the contiguous renderfarm output.  
// Then we restart the background renderfarm at the
// lesser of the positions.  You can't conditionally restart only 
// if one of the current jobs was after the position because you need a new EDL.

// The two problems to emerge are which job is the last job in the contiguous
// set of finished jobs and if position of change is before the last job, 
// how to truncate and restart the output file.

// It's easy to use image sequences as the output file to solve the 
// file truncation problem.
// Figuring out the end of the contiguous output means recording the
// state of every output file and constructing a kind of EDL for the 
// background output as certain output files cluster together.
// This is needed anyway for playback.

#include "arraylist.h"
#include "bcwindowbase.inc"
#include "brender.inc"
#include "datatype.h"
#include "condition.inc"
#include "edl.inc"
#include "linklist.h"
#include "maplist.h"
#include "mutex.inc"
#include "mwindow.inc"
#include "packagedispatcher.inc"
#include "preferences.inc"
#include "renderfarm.inc"
#include "thread.h"
#include "bctimer.inc"


class BRender : public Thread
{
public:
	BRender();
	~BRender();

// Give the last position of the EDL which hasn't changed.
// We copy the EDL and restart rendering at the lesser of position and
// our position.
	void restart(EDL *edl);
// Stop background rendering for a foreground render.  This blocks until 
// it really stops.
	void stop();

// Allocate map with locking
	void allocate_map(ptstime brender_start, ptstime start, ptstime end);
// Mark a frame as finished
	void set_video_map(ptstime start, ptstime end);
// Called when rendering ends
	void render_done();

	void initialize();
	void run();

// Simple map of finished chunks
	MapList videomap;
	Mutex *map_lock;

// Invalidate the map until reallocation when a new edit operation is performed.
	int map_valid;

// Wait until stop commands are finished
	Condition *completion_lock;
	BRenderThread *thread;
// PID of master node for killing.
	pid_t master_pid;
// Path of socket
	char socket_path[BCTEXTLEN];
// Arguments for execvp
	char *arguments[4];
	Timer *timer;
};

class BRenderCommand
{
public:
	BRenderCommand();

	EDL *edl;

	enum
	{
		BRENDER_NONE,
		BRENDER_RESTART,
		BRENDER_STOP
	};

	int command;

// The location of the last change.
	double position;
// The earliest point to include in background rendering would be stored in the
// EDL.
};

class BRenderThread : public Thread
{
public:
	BRenderThread(BRender *brender);
	~BRenderThread();

	int is_done(int do_lock);
	void send_command(BRenderCommand *command);
	void run();
	void stop();
	void start();
	void initialize();

	BRender *brender;
	BRenderCommand *command_queue;
	BRenderCommand *command;
	Condition *input_lock;
	Mutex *thread_lock;
// Render farm server.  Deleted when stopped.  Created when restarted.
	RenderFarmServer *farm_server;
	PackageDispatcher *packages;
// Copy of preferences with modified render farm.
	Preferences *preferences;
// Cpoy of current edl
	EDL *edl;
// Render farm polls these.
	int farm_result;
	double fps_result;

	int done;
};

#endif

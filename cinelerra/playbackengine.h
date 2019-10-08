
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

#ifndef PLAYBACKENGINE_H
#define PLAYBACKENGINE_H

#include "arraylist.h"
#include "audiodevice.inc"
#include "cache.inc"
#include "canvas.inc"
#include "condition.inc"
#include "datatype.h"
#include "bchash.inc"
#include "edl.inc"
#include "mutex.inc"
#include "tracking.inc"
#include "renderengine.inc"
#include "thread.h"
#include "bctimer.h"
#include "transportcommand.inc"

#define MAX_COMMAND_QUEUE 8

class PlaybackEngine : public Thread
{
public:
	PlaybackEngine(Canvas *output);
	virtual ~PlaybackEngine();

	void create_render_engine();
	void delete_render_engine();
	void arm_render_engine();
	void start_render_engine();
	void wait_render_engine();
	void create_cache();
	void perform_change();
// Set wait_tracking for events that change the cursor location but
// be sure to unlock the windows
	void interrupt_playback(int wait_tracking = 0);
// Get levels for tracking.  Return 0 if no audio.
	int get_output_levels(double *levels, ptstime pts);
	int get_module_levels(double *levels, ptstime pts);
// The MWindow starts the playback cursor loop
// The other windows start a slider loop
// For pausing only the cursor is run
	virtual void init_cursor() {};
	virtual void stop_cursor() {};
	virtual int brender_available(ptstime position) { return 0; };
// For normal playback tracking and the cursor are started
	virtual void init_tracking();
	virtual void stop_tracking(ptstime position = -1);
// The playback cursor calls this to calculate the current tracking position
	virtual ptstime get_tracking_position();

	void send_command(int command, EDL *new_edl = 0, int options = 0);

	void run();

// Maintain caches through console changes
	CICache *audio_cache, *video_cache;
// Maintain playback cursor on GUI
	int tracking_active;
// Block returns until tracking loop is finished
	Condition *tracking_done;
// Wait until thread has started
	Condition *start_lock;

	Canvas *output;
// Currently executing command
	TransportCommand *command;
// Render engine
	RenderEngine *render_engine;

// Used by label commands to get current position
	int is_playing_back;
private:
// Tracking variables updated from rendering position
	ptstime tracking_position;
	ptstime tracking_start;
	ptstime video_pts;
	int done;
	Mutex *cmds_lock;
	Condition *playback_lock;
	int used_cmds;
	TransportCommand *cmds[MAX_COMMAND_QUEUE];
};

#endif

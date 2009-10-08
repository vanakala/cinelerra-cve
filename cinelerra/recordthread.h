
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

#ifndef RECORDTHREAD_H
#define RECORDTHREAD_H

#include "condition.inc"
#include "drivesync.inc"
#include "file.inc"
#include "mutex.inc"
#include "record.inc"
#include "recordaudio.inc"
#include "recordvideo.inc"
#include "thread.h"
#include "bctimer.inc"

// Synchronously handle recording and monitoring

class RecordThread : public Thread
{
public:
	RecordThread(MWindow *mwindow, Record *record);
	~RecordThread();

	int create_objects();
	int start_recording(int monitor, int context);
	int stop_recording(int resume_monitor);
	int pause_recording();
	int resume_recording();
	int64_t sync_position();
	void do_cron();

	void run();

	int quit_when_completed;
	RecordAudio *record_audio;
	RecordVideo *record_video;
// Whether to write data to disk
	int monitor;
// Whether to open audio device or record single frame
	int single_frame;
// CONTEXT_INTERACTIVE, CONTEXT_BATCH, CONTEXT_SINGLEFRAME
	int context;
	Timer *record_timer;
	int engine_done;
	int resume_monitor;
// Cron behavior
	double last_seconds;
	int last_day;

private:
	MWindow *mwindow;
	Record *record;
	File *file;
	Condition *pause_lock, *startup_lock, *loop_lock;
// Lock termination variables to positions where batches aren't being started.
	Mutex *state_lock;
// Override the operating system
	DriveSync *drivesync;
};


#endif


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
#include "audiodevice.h"
#include "batch.h"
#include "bcsignals.h"
#include "condition.h"
#include "drivesync.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "mutex.h"
#include "mwindow.h"
#include "record.h"
#include "recordaudio.h"
#include "recordgui.h"
#include "recordthread.h"
#include "recordvideo.h"
#include "bctimer.h"
#include "videodevice.h"



#define RING_BUFFERS 2


RecordThread::RecordThread(MWindow *mwindow, Record *record)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	this->record = record;
	quit_when_completed = 0;
	record_timer = new Timer;
	record_audio = 0;
	record_video = 0;
	pause_lock = new Condition(1, "RecordThread::pause_lock");
	startup_lock = new Condition(1, "RecordThread::startup_lock");
	loop_lock = new Condition(1, "RecordThread::loop_lock");
	state_lock = new Mutex("RecordThread::state_lock");
}

RecordThread::~RecordThread()
{
SET_TRACE
	delete record_audio;
SET_TRACE
	delete record_video;
SET_TRACE
	delete record_timer;
SET_TRACE
	delete pause_lock;
SET_TRACE
	delete startup_lock;
SET_TRACE
	delete loop_lock;
SET_TRACE
	delete state_lock;
SET_TRACE
}

int RecordThread::create_objects()
{
	if(record->default_asset->audio_data) 
		record_audio = new RecordAudio(mwindow, 
			record, 
			this);

	if(record->default_asset->video_data) 
		record_video = new RecordVideo(mwindow, 
			record, 
			this);
	engine_done = 0;
	return 0;
}

int RecordThread::start_recording(int monitor, int context)
{
	engine_done = 0;
	this->monitor = monitor;
	this->context = context;
	resume_monitor = !monitor;
	loop_lock->lock("RecordThread::start_recording");
// Startup lock isn't 
	startup_lock->lock("RecordThread::start_recording");


	Thread::start();
	startup_lock->lock("RecordThread::start_recording");
	startup_lock->unlock();
	return 0;
}

int RecordThread::stop_recording(int resume_monitor)
{
SET_TRACE
// Stop RecordThread while waiting for batch
	state_lock->lock("RecordThread::stop_recording");

	engine_done = 1;
	if(monitor)
	{
		pause_lock->unlock();
	}

SET_TRACE
	this->resume_monitor = resume_monitor;
// In the monitor engine, stops the engine.
// In the recording engine, causes the monitor engine not to be restarted.
// Video thread stops the audio thread itself
// printf("RecordThread::stop_recording 1\n");
SET_TRACE
	if(record_video)
	{
		record_video->batch_done = 1;
		state_lock->unlock();
		record_video->stop_recording();
	}
	else
	if(record_audio && context != CONTEXT_SINGLEFRAME) 
	{
		record_audio->batch_done = 1;
		state_lock->unlock();
		record_audio->stop_recording();
	}

SET_TRACE
	Thread::join();
SET_TRACE
	return 0;
}

int RecordThread::pause_recording()
{
// Stop the thread before finishing the loop
	pause_lock->lock("RecordThread::pause_recording");

	state_lock->lock("RecordThread::pause_recording");
	if(record->default_asset->video_data)
	{
		record_video->batch_done = 1;
	}
	else if (record->default_asset->audio_data)
	{
		record_audio->batch_done = 1;
	}
	state_lock->unlock();
// Stop the recordings
	if(record->default_asset->audio_data && context != CONTEXT_SINGLEFRAME)
		record_audio->stop_recording();
	if(record->default_asset->video_data)
		record_video->stop_recording();

// Wait for thread to stop before closing devices
	loop_lock->lock("RecordThread::pause_recording");
	loop_lock->unlock();



	record->close_input_devices(monitor);
//printf("RecordThread::pause_recording 2\n");
	record->capture_state = IS_DONE;
	return 0;
}

int RecordThread::resume_recording()
{
//printf("RecordThread::resume_recording 1\n");
	if(record_video)
	{
		record_video->batch_done = 0;
	}
	else if (record_audio)
	{
		record_audio->batch_done = 0;
	}
	loop_lock->lock("RecordThread::resume_recording");
	pause_lock->unlock();
//printf("RecordThread::resume_recording 2\n");
	return 0;
}

int64_t RecordThread::sync_position()
{
	if(record->default_asset->audio_data)
		return record_audio->sync_position();
	else
		return (int64_t)((float)record_timer->get_difference() / 
			1000 * 
			record->default_asset->sample_rate + 0.5);
}

void RecordThread::do_cron()
{
	do{
		double position = record->current_display_position();
		int day;
		double seconds;
		

// Batch already started
		if(position > 0)
		{
			break;
		}
		else
// Delay until start of batch
		{
			record->get_current_time(seconds, day);

// Wildcard
			if(record->get_current_batch()->start_day == 7)
				day = record->get_current_batch()->start_day;
// Start recording
			if(record->get_current_batch()->start_day == day &&
				record->get_current_batch()->start_time >= last_seconds &&
				record->get_current_batch()->start_time <= seconds)
			{
				break;
			}

// 			record->record_gui->lock_window();
// 			record->record_gui->flash_batch();
// 			record->record_gui->unlock_window();
		}

		last_seconds = seconds;
		last_day = day;
		if(!engine_done) usleep(BATCH_DELAY);
		if(!engine_done)
		{
			record->record_gui->lock_window("RecordThread::do_cron");
			record->record_gui->flash_batch();
			record->record_gui->unlock_window();
		}
	}while(!engine_done);
}



void RecordThread::run()
{
	int rewinding_loop = 0;
	startup_lock->unlock();
	record->get_current_time(last_seconds, last_day);


	do
	{
// Prepare next batch
		if(context == CONTEXT_BATCH &&
			!rewinding_loop)
		{
			do_cron();
		}

		state_lock->lock("RecordThread::run");
// Test for stopped while waiting
		if(!engine_done)
		{
			
			rewinding_loop = 0;

// Batch context needs to open the device here.  Interactive and singleframe
// contexts need to open in Record::start_recording to allow full duplex.
			if(context == CONTEXT_BATCH)
			{
// Delete output file before opening the devices to avoid buffer overflow.
TRACE("RecordThread::run 1");
				record->delete_output_file();
TRACE("RecordThread::run 2");
				record->open_input_devices(0, context);
TRACE("RecordThread::run 3");
			}

// Switch interactive recording to batch recording
// to get delay before next batch
			if(!monitor && context == CONTEXT_INTERACTIVE)
				context = CONTEXT_BATCH;

			if(!monitor)
			{
// This draws to RecordGUI, incidentally
TRACE("RecordThread::run 4");
				record->open_output_file();
TRACE("RecordThread::run 5");
				if(mwindow->edl->session->record_sync_drives)
				{
					drivesync = new DriveSync;
					drivesync->start();
				}
				else
					drivesync = 0;

				record->get_current_batch()->recorded = 1;
TRACE("RecordThread::run 6");

// Open file threads here to keep loop synchronized
				if(record->default_asset->audio_data && 
					context != CONTEXT_SINGLEFRAME)
				{
					int buffer_size, fragment_size;
					record->get_audio_write_length(buffer_size, 
						fragment_size);
					record->file->start_audio_thread(buffer_size, RING_BUFFERS);
				}
TRACE("RecordThread::run 7");

				if(record->default_asset->video_data)
					record->file->start_video_thread(mwindow->edl->session->video_write_length,
						record->vdevice->get_best_colormodel(record->default_asset),
						RING_BUFFERS,
						record->vdevice->is_compressed(1, 0));
TRACE("RecordThread::run 8");
			}

// Reset synchronization  counters
			record->get_current_batch()->session_samples = 0;
			record->get_current_batch()->session_frames = 0;
			record_timer->update();

// Do initialization
TRACE("RecordThread::run 9");
			if(record->default_asset->audio_data && context != CONTEXT_SINGLEFRAME)
				record_audio->arm_recording();
TRACE("RecordThread::run 10");
			if(record->default_asset->video_data)
				record_video->arm_recording();
TRACE("RecordThread::run 11");
			state_lock->unlock();

// Trigger loops

			if(record->default_asset->audio_data && context != CONTEXT_SINGLEFRAME)
				record_audio->start_recording();
TRACE("RecordThread::run 12");
			if(record->default_asset->video_data)
				record_video->start_recording();
TRACE("RecordThread::run 13");


			if(record->default_asset->audio_data && context != CONTEXT_SINGLEFRAME)
				record_audio->Thread::join();
TRACE("RecordThread::run 14");
			if(record->default_asset->video_data)
				record_video->Thread::join();
TRACE("RecordThread::run 15");

// Stop file threads here to keep loop synchronized
			if(!monitor)
			{
				if(drivesync) drivesync->done = 1;
TRACE("RecordThread::run 16");
				if(record->default_asset->audio_data && context != CONTEXT_SINGLEFRAME)
					record->file->stop_audio_thread();
TRACE("RecordThread::run 17");
				if(record->default_asset->video_data)
					record->file->stop_video_thread();
TRACE("RecordThread::run 18");

// Update asset info
				record->get_current_batch()->get_current_asset()->audio_length = 
					record->get_current_batch()->total_samples;
				record->get_current_batch()->get_current_asset()->video_length = 
					record->get_current_batch()->total_frames;

// Reset the loop flag and rewind files for the next loop
				if(!engine_done)
				{
// Rewind loop if in loop mode
					if(record->get_current_batch()->record_mode == RECORD_LOOP)
					{
// Don't close devices when rewinding a loop
						record->rewind_file();
						record->get_current_batch()->session_samples = 0;
						record->get_current_batch()->session_frames = 0;
						rewinding_loop = 1;
					}
					else
// Advance batch if not terminated by user and not single frame and continue loop
					if(record->get_next_batch() >= 0 && context != CONTEXT_SINGLEFRAME)
					{
						record->activate_batch(record->get_next_batch(), 0);
						record->close_input_devices(monitor);
					}
					else
// End loop
					{
						engine_done = 1;
					}
				}
TRACE("RecordThread::run 20");

				if(drivesync) delete drivesync;
			}
SET_TRACE
		}
		else
		{
			state_lock->unlock();
		}

SET_TRACE
// Wait for thread to stop before closing devices
		loop_lock->unlock();
		if(monitor)
		{
// Pause until monitor is resumed
			pause_lock->lock("RecordThread::run");
			pause_lock->unlock();
		}
SET_TRACE
	}while(!engine_done);

SET_TRACE
	record->close_input_devices(monitor);
SET_TRACE

// Resume monitoring only if not a monitor ourselves
	if(!monitor)
	{
		record->stop_duplex();
		if(resume_monitor) record->resume_monitor();
	}
	else
	{
		record->capture_state = IS_DONE;
	}
SET_TRACE
}


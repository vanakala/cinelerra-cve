#include "assets.h"
#include "audiodevice.h"
#include "batch.h"
#include "drivesync.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "mwindow.h"
#include "record.h"
#include "recordaudio.h"
#include "recordgui.h"
#include "recordthread.h"
#include "recordvideo.h"
#include "timer.h"
#include "videodevice.h"


RecordThread::RecordThread(MWindow *mwindow, Record *record)
 : Thread()
{
	this->mwindow = mwindow;
	this->record = record;
	quit_when_completed = 0;
	record_timer = new Timer;
	record_audio = 0;
	record_video = 0;
}

RecordThread::~RecordThread()
{
	delete record_timer;
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
	loop_lock.lock();
// Startup lock isn't 
	startup_lock.lock();
	completion_lock.lock();


	set_synchronous(0);
	Thread::start();
	startup_lock.lock();
	startup_lock.unlock();
	return 0;
}

int RecordThread::stop_recording(int resume_monitor)
{
	engine_done = 1;
	this->resume_monitor = resume_monitor;
// In the monitor engine, stops the engine.
// In the recording engine, causes the monitor engine not to be restarted.
// Video thread stops the audio thread itself
	if(record_video)
	{
//printf("RecordThread::stop_recording 1\n");
		record_video->batch_done = 1;
//printf("RecordThread::stop_recording 1\n");
		record_video->stop_recording();
//printf("RecordThread::stop_recording 2\n");
	}
	else
	if(record_audio && context != CONTEXT_SINGLEFRAME) 
	{
//printf("RecordThread::stop_recording 3\n");
		record_audio->batch_done = 1;
//printf("RecordThread::stop_recording 4\n");
		record_audio->stop_recording();
//printf("RecordThread::stop_recording 5\n");
	}


//printf("RecordThread::stop_recording 6\n");
	completion_lock.lock();
	completion_lock.unlock();
//printf("RecordThread::stop_recording 7\n");
	return 0;
}

int RecordThread::pause_recording()
{
// Stop the thread before finishing the loop
//printf("RecordThread::pause_recording 1\n");
	pause_lock.lock();
//printf("RecordThread::pause_recording 1\n");

	if(record->default_asset->video_data)
	{
		record_video->batch_done = 1;
	}
	else
	{
		record_audio->batch_done = 1;
	}
//printf("RecordThread::pause_recording 1\n");
// Stop the recordings
	if(record->default_asset->audio_data && context != CONTEXT_SINGLEFRAME)
		record_audio->stop_recording();
	if(record->default_asset->video_data)
		record_video->stop_recording();
//printf("RecordThread::pause_recording 1\n");

// Wait for thread to stop before closing devices
	loop_lock.lock();
	loop_lock.unlock();
//printf("RecordThread::pause_recording 1\n");



	record->close_input_devices();
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
	else
	{
		record_audio->batch_done = 0;
	}
	loop_lock.lock();
	pause_lock.unlock();
//printf("RecordThread::resume_recording 2\n");
	return 0;
}

long RecordThread::sync_position()
{
	if(record->default_asset->audio_data)
		return record_audio->sync_position();
	else
		return (long)((float)record_timer->get_difference() / 
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
			record->record_gui->lock_window();
			record->record_gui->flash_batch();
			record->record_gui->unlock_window();
		}
	}while(!engine_done);
}



void RecordThread::run()
{
	int rewinding_loop = 0;
	startup_lock.unlock();
	record->get_current_time(last_seconds, last_day);
	

	do
	{
// Prepare next batch
		if(context == CONTEXT_BATCH &&
			!rewinding_loop)
		{
			do_cron();
		}

// Stopped while waiting
		if(!engine_done)
		{
			rewinding_loop = 0;
//printf("RecordThread::run 6\n");

// Batch context needs to open the device here.  Interactive and singleframe
// contexts need to open in Record::start_recording to allow full duplex.
			if(context == CONTEXT_BATCH)
			{
// Delete output file before opening the devices to avoid buffer overflow.
				record->delete_output_file();
				record->open_input_devices(0, context);
			}
//printf("RecordThread::run 7 %d\n", monitor);

// Switch interactive recording to batch recording
// to get delay before next batch
			if(!monitor && context == CONTEXT_INTERACTIVE)
				context = CONTEXT_BATCH;

			if(!monitor)
			{
// This draws to RecordGUI, incidentally
				record->open_output_file();
				if(mwindow->edl->session->record_sync_drives)
				{
					drivesync = new DriveSync;
					drivesync->start();
				}
				else
					drivesync = 0;

				record->get_current_batch()->recorded = 1;

// Open file threads here to keep loop synchronized
				if(record->default_asset->audio_data && 
					context != CONTEXT_SINGLEFRAME)
				{
					long buffer_size, fragment_size;
					record->get_audio_write_length(buffer_size, fragment_size);
					record->file->start_audio_thread(buffer_size, RING_BUFFERS);
				}

				if(record->default_asset->video_data)
					record->file->start_video_thread(mwindow->edl->session->video_write_length,
						record->vdevice->get_best_colormodel(record->default_asset),
						RING_BUFFERS,
						record->vdevice->is_compressed());
			}
//printf("RecordThread::run 8\n");

// Reset synchronization  counters
			record->get_current_batch()->session_samples = 0;
			record->get_current_batch()->session_frames = 0;
			record_timer->update();

// Do initialization
			if(record->default_asset->audio_data && context != CONTEXT_SINGLEFRAME)
				record_audio->arm_recording();
			if(record->default_asset->video_data)
				record_video->arm_recording();
//printf("RecordThread::run 9\n");

// Trigger loops

			if(record->default_asset->audio_data && context != CONTEXT_SINGLEFRAME)
				record_audio->start_recording();
			if(record->default_asset->video_data)
				record_video->start_recording();

//printf("RecordThread::run 10\n");

			if(record->default_asset->audio_data && context != CONTEXT_SINGLEFRAME)
				record_audio->join();
			if(record->default_asset->video_data)
				record_video->join();
//printf("RecordThread::run 11\n");

// Stop file threads here to keep loop synchronized
			if(!monitor)
			{
				if(drivesync) drivesync->done = 1;
				if(record->default_asset->audio_data && context != CONTEXT_SINGLEFRAME)
					record->file->stop_audio_thread();
				if(record->default_asset->video_data)
					record->file->stop_video_thread();

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
						record->close_input_devices();
					}
					else
// End loop
					{
						engine_done = 1;
					}
				}

				if(drivesync) delete drivesync;
			}
//printf("RecordThread::run 12\n");
		}

// Wait for thread to stop before closing devices
		loop_lock.unlock();
		if(monitor)
		{
// Pause until monitor is resumed
			pause_lock.lock();
			pause_lock.unlock();
		}
	}while(!engine_done);

//printf("RecordThread::run 13\n");
	record->close_input_devices();

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
	completion_lock.unlock();
}


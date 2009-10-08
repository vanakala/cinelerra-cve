
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
#include "batch.h"
#include "bcsignals.h"
#include "clip.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "errorbox.h"
#include "file.h"
#include "filethread.h"
#include "language.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "preferences.h"
#include "quicktime.h"
#include "record.h"
#include "recordaudio.h"
#include "recordgui.h"
#include "recordthread.h"
#include "recordvideo.h"
#include "recordmonitor.h"
#include "units.h"
#include "vframe.h"
#include "videodevice.h"

#include <unistd.h>


RecordVideo::RecordVideo(MWindow *mwindow,
	Record *record, 
	RecordThread *record_thread)
 : Thread(1, 0, 0)
{
	reset_parameters();
	this->mwindow = mwindow;
	this->record = record;
	this->record_thread = record_thread; 
	this->gui = record->record_gui;
	unhang_lock = new Mutex("RecordVideo::unhang_lock");
	trigger_lock = new Condition(1, "RecordVideo::trigger_lock");
	capture_frame = 0;
	frame_ptr = 0;
}

RecordVideo::~RecordVideo()
{
	delete unhang_lock;
	delete trigger_lock;
// These objects are shared with the file if recording.
	if(record_thread->monitor)
	{
		if(frame_ptr)
		{
			if(frame_ptr[0]) delete [] frame_ptr[0];
			delete [] frame_ptr;
		}
		delete capture_frame;
	}
}

void RecordVideo::reset_parameters()
{
	write_result = 0;
	grab_result = 0;
	total_dropped_frames = 0;
	dropped_frames = 0;
	last_dropped_frames = 0;
	record_start = 0;
	buffer_position = 0;
	batch_done = 0;
}

int RecordVideo::arm_recording()
{
	reset_parameters();
// Resume next file in a series by incrementing existing counters
	if(record_thread->monitor)
		buffer_size = 1;
	else
		buffer_size = mwindow->edl->session->video_write_length;

	trigger_lock->lock("RecordVideo::arm_recording");
	Thread::start();

	return 0;
}

void RecordVideo::start_recording()
{
	trigger_lock->unlock();
}

int RecordVideo::stop_recording()
{
// Device won't exist if interrupting a cron job
	if(record->vdevice)
	{
// Interrupt IEEE1394 crashes
		record->vdevice->interrupt_crash();

// Interrupt video4linux crashes
		if(record->vdevice->get_failed())
		{
			Thread::end();
			Thread::join();

			cleanup_recording();
		}
	}
// Joined in RecordThread
	return 0;
}


int RecordVideo::cleanup_recording()
{
	if(!record_thread->monitor)
	{
// write last buffer
		write_buffer(1);
// stop file I/O
	}
	else
	{
// RecordMonitorThread still needs capture_frame if uncompressed.
// 		delete [] frame_ptr[0];
// 		delete [] frame_ptr;
// 		delete capture_frame;
	}
	return 0;
}

void RecordVideo::get_capture_frame()
{
	if(!capture_frame)
	{
		if(record->fixed_compression)
		{
			capture_frame = new VFrame;
		}
		else
		{
			capture_frame = new VFrame(0, 
				record->default_asset->width, 
				record->default_asset->height, 
				record->vdevice->get_best_colormodel(record->default_asset));
//printf("RecordVideo::get_capture_frame %d %d\n", capture_frame->get_w(), capture_frame->get_h());
		}
		frame_ptr = new VFrame**[1];
		frame_ptr[0] = new VFrame*[1];
		frame_ptr[0][0] = capture_frame;
	}
}


void RecordVideo::run()
{
	write_result = 0;
	grab_result = 0;

// Thread out the I/O
	if(!record_thread->monitor)
	{
		record_start = record->file->get_video_position(record->default_asset->frame_rate);
		frame_ptr = record->file->get_video_buffer();
	}
	else
	{
		get_capture_frame();
	}

// Number of frames for user to know about.
	gui->total_dropped_frames = 0;
	gui->update_dropped_frames(0);


// Wait for trigger
	trigger_lock->lock("RecordVideo::run");
	trigger_lock->unlock();

	while(!batch_done && 
		!write_result)
	{
// Synchronize with audio or timer
		dropped_frames = 0;
		next_sample = (int64_t)((float)record->get_current_batch()->session_frames / 
			record->default_asset->frame_rate * 
			record->default_asset->sample_rate);
 		current_sample = record->sync_position();


		if(current_sample < next_sample && current_sample > 0)
		{
// Too early.
			delay = (int64_t)((float)(next_sample - current_sample) / 
				record->default_asset->sample_rate * 
				1000);
// Sanity check and delay.
// In 2.6.7 this doesn't work.  For some reason, probably buffer overflowing,
// it causes the driver to hang up momentarily so we try to only delay
// when really really far ahead.
			if(delay < 2000 && delay > 0) 
			{
				delayer.delay(delay);
			}
			gui->update_dropped_frames(0);
			last_dropped_frames = 0;
		}
		else
		if(current_sample > 0 && !record_thread->monitor)
		{
// Too late.
			dropped_frames = (int64_t)((float)(current_sample - next_sample) / 
				record->default_asset->sample_rate * 
				record->default_asset->frame_rate);
			if(dropped_frames != last_dropped_frames)
			{
				gui->update_dropped_frames(dropped_frames);
				last_dropped_frames = dropped_frames;
			}
			last_dropped_frames = dropped_frames;
		}



// Capture a frame
		if(!batch_done)
		{
// Grab frame for recording
			if(!record_thread->monitor)
			{
				capture_frame = frame_ptr[0][buffer_position];
				record->vdevice->set_field_order(record->reverse_interlace);
				read_buffer();
				record->get_current_batch()->current_frame++;
				record->get_current_batch()->total_frames = 
					MAX(record->get_current_batch()->current_frame, record->get_current_batch()->total_frames);
				record->get_current_batch()->session_frames++;
				if(!grab_result) buffer_position++;

// Update the position indicator
				gui->update_position(record->current_display_position());
			}
			else
// Grab frame for monitoring
			if(record->monitor_video)
			{
				record->vdevice->set_field_order(record->reverse_interlace);
				record->get_current_batch()->session_frames++;

				read_buffer();
			}
			else
// Brief pause to keep CPU from burning up
			{
				Timer::delay(250);
			}
		}

// Monitor the frame if monitoring
// printf("RecordVideo::run %p %d %d %d\n", 
// capture_frame->get_data(), 
// record->monitor_video, 
// batch_done, 
// grab_result);
		if(capture_frame->get_data() && 
			record->monitor_video && 
			!batch_done && 
			!grab_result)
		{
			record->record_monitor->update(capture_frame);
		}

// Duplicate a frame if behind
		if(!record_thread->monitor && 
			record->fill_frames && 
			!batch_done && 
			dropped_frames > 1)
		{
			VFrame *last_frame = capture_frame;
// Write frame 1
			if(buffer_position >= buffer_size) write_buffer(0);
// Copy to frame 2
			capture_frame = frame_ptr[0][buffer_position];
			capture_frame->copy_from(last_frame);
			record->get_current_batch()->current_frame++;
			record->get_current_batch()->total_frames = 
				MAX(record->get_current_batch()->current_frame, record->get_current_batch()->total_frames);
			record->get_current_batch()->session_frames++;
			buffer_position++;
		}

// Compress a batch of frames or write the second frame if filling
		if(!record_thread->monitor && buffer_position >= buffer_size)
		{
			write_buffer(0);
		}

		if(!record_thread->monitor && 
			!batch_done &&
			!write_result)
		{
// Handle recording contexts
			if(record_thread->context == CONTEXT_SINGLEFRAME)
			{
				batch_done = 1;
			}
			else
// Handle recording modes delegated to the thread
			switch(record->get_current_batch()->record_mode)
			{
				case RECORD_TIMED:
					if(record->current_display_position() > *record->current_duration())
						batch_done = 1;
					break;
				case RECORD_LOOP:
					if(record->current_display_position() > *record->current_duration())
						batch_done = 1;
					break;
				case RECORD_SCENETOSCENE:
					break;
			}
		}

		if(write_result)
		{
			batch_done = 1;
		}
	}

//TRACE("RecordVideo::run 1");
// Update dependant threads
	if(record->default_asset->audio_data)
	{
		record_thread->record_audio->batch_done = 1;
// Interrupt driver for IEEE1394
		record_thread->record_audio->stop_recording();
	}

//TRACE("RecordVideo::run 2");

	if(write_result)
	{
		if(!record_thread->monitor)
		{
			ErrorBox error_box(PROGRAM_NAME ": Error",
				mwindow->gui->get_abs_cursor_x(1),
				mwindow->gui->get_abs_cursor_y(1));
			error_box.create_objects(_("No space left on disk."));
			error_box.run_window();
			batch_done = 1;
		}
	}

	cleanup_recording();
SET_TRACE
}

void RecordVideo::read_buffer()
{
	grab_result = record->vdevice->read_buffer(capture_frame);


// Get field offset for monitor
	if(!strncmp(record->default_asset->vcodec, QUICKTIME_MJPA, 4) &&
		record->vdevice->is_compressed(0, 1))
	{
		unsigned char *data = capture_frame->get_data();
		int64_t size = capture_frame->get_compressed_size();
		int64_t allocation = capture_frame->get_compressed_allocated();

		if(data)
		{
			int64_t field2_offset = mjpeg_get_field2(data, size);
			capture_frame->set_compressed_size(size);
			capture_frame->set_field2_offset(field2_offset);
		}
	}
}

void RecordVideo::write_buffer(int skip_new)
{
	write_result = record->file->write_video_buffer(buffer_position);
	buffer_position = 0;
	if(!skip_new && !write_result) 
		frame_ptr = record->file->get_video_buffer();
}

void RecordVideo::rewind_file()
{
	write_buffer(1);
	record->file->stop_video_thread();
	record->file->set_video_position(0, record->default_asset->frame_rate);
	record->file->start_video_thread(buffer_size,
		record->vdevice->get_best_colormodel(record->default_asset),
		2,
		record->vdevice->is_compressed(1, 0));
	frame_ptr = record->file->get_video_buffer();
	record->get_current_batch()->current_frame = 0;
	record->get_current_batch()->current_sample = 0;
	record->get_current_batch()->session_frames = 0;
	record->get_current_batch()->session_samples = 0;
	gui->update_position(0);
}

int RecordVideo::unhang_thread()
{
printf("RecordVideo::unhang_thread\n");
	Thread::end();
}


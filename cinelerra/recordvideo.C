#include "assets.h"
#include "batch.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "errorbox.h"
#include "file.h"
#include "filethread.h"
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
 : Thread()
{
	reset_parameters();
	this->mwindow = mwindow;
	this->record = record;
	this->record_thread = record_thread; 
	this->gui = record->record_gui;
}

RecordVideo::~RecordVideo()
{
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

	set_synchronous(1);
	trigger_lock.lock();
	Thread::start();

	return 0;
}

void RecordVideo::start_recording()
{
	trigger_lock.unlock();
}

int RecordVideo::stop_recording()
{
// Device won't exist if interrupting a cron job
	if(record->vdevice)
	{
//printf("RecordVideo::stop_recording 1 %p\n", record->vdevice);
// Interrupt IEEE1394 crashes
		record->vdevice->interrupt_crash();
//printf("RecordVideo::stop_recording 1\n");

// Interrupt video4linux crashes
		if(record->vdevice->get_failed())
		{
//printf("RecordVideo::stop_recording 2\n");
			Thread::end();
			Thread::join();
//printf("RecordVideo::stop_recording 3\n");

			cleanup_recording();
//printf("RecordVideo::stop_recording 4\n");
		}
	}
//printf("RecordVideo::stop_recording 5\n");
	return 0;
}


int RecordVideo::cleanup_recording()
{
	if(!record_thread->monitor)
	{
//printf("RecordVideo::cleanup_recording 1\n");
// write last buffer
		write_buffer(1);
// stop file I/O
//printf("RecordVideo::cleanup_recording 2\n");
	}
	else
	{
		delete frame_ptr[0];
		delete frame_ptr;
		delete capture_frame;
	}
	return 0;
}

void RecordVideo::get_capture_frame()
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
	trigger_lock.lock();
	trigger_lock.unlock();

//printf("RecordVideo::run 1 %d\n", record_thread->monitor);

	while(!batch_done && 
		!write_result)
	{
// Synchronize with audio or timer
//printf("RecordVideo::run 1\n");
		dropped_frames = 0;
		next_sample = (long)((float)record->get_current_batch()->session_frames / 
			record->default_asset->frame_rate * 
			record->default_asset->sample_rate);
//printf("RecordVideo::run 1\n");
 		current_sample = record->sync_position();

//printf("RecordVideo::run 2\n");


		if(current_sample < next_sample && current_sample > 0)
		{
// Too early.
			delay = (long)((float)(next_sample - current_sample) / 
				record->default_asset->sample_rate * 
				1000  
//              / 2
				);
// Sanity check and delay.
			if(delay < 2000 && delay > 0) delayer.delay(delay);
			gui->update_dropped_frames(0);
			last_dropped_frames = 0;
		}
		else
		if(current_sample > 0 && !record_thread->monitor)
		{
// Too late.
			dropped_frames = (long)((float)(current_sample - next_sample) / 
				record->default_asset->sample_rate * 
				record->default_asset->frame_rate);
			if(dropped_frames != last_dropped_frames)
			{
				gui->update_dropped_frames(dropped_frames);
				last_dropped_frames = dropped_frames;
			}
			last_dropped_frames = dropped_frames;
		}

//printf("RecordVideo::run 3\n");
// Capture a frame
		if(!batch_done)
		{
// Grab frame for recording
			if(!record_thread->monitor)
			{
//printf("RecordVideo::run 4\n");
				capture_frame = frame_ptr[0][buffer_position];
//printf("RecordVideo::run 5 %d\n", capture_frame->get_color_model());
				record->vdevice->set_field_order(record->reverse_interlace);
//printf("RecordVideo::run 6\n");
				read_buffer();
//printf("RecordVideo::run 7\n");
				record->get_current_batch()->current_frame++;
				record->get_current_batch()->total_frames = 
					MAX(record->get_current_batch()->current_frame, record->get_current_batch()->total_frames);
				record->get_current_batch()->session_frames++;
//printf("RecordVideo::run 4\n");
				if(!grab_result) buffer_position++;
//printf("RecordVideo::run 5 %ld\n", record_thread->session_frames);

// Update the position indicator
				gui->update_position(record->current_display_position(),
					record->current_display_length());
			}
			else
// Grab frame for monitoring
			if(record->monitor_video)
			{
				record->vdevice->set_field_order(record->reverse_interlace);
				record->get_current_batch()->session_frames++;

//printf("RecordVideo::run 4\n");
				read_buffer();
//printf("RecordVideo::run 5\n");
			}
			else
// Brief pause to keep CPU from burning up
			{
				Timer timer;
				timer.delay(250);
			}
		}

//printf("RecordVideo::run 10\n");
// Monitor the frame if monitoring
		if(capture_frame->get_data() && 
			record->monitor_video && 
			!batch_done && 
			!grab_result)
			record->record_monitor->update(capture_frame);
// printf("RecordVideo::run 11 %d %d %d %d\n", record_thread->monitor,
// 			record->fill_frames, 
// 			batch_done,  
// 			dropped_frames);

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
//printf("RecordVideo::run 12\n");
//printf("RecordVideo::run 12 %d %f %f\n", record->get_current_batch()->record_mode, record->current_display_position(), *record->current_duration());

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
//printf("RecordVideo::run 13 %d\n", write_result);

		if(write_result)
		{
			batch_done = 1;
		}
	}
//printf("RecordVideo::run 14 %d\n", write_result);

// Update dependant threads
	if(record->default_asset->audio_data)
	{
		record_thread->record_audio->batch_done = 1;
// Interrupt driver for IEEE1394
		record_thread->record_audio->stop_recording();
	}
//printf("RecordVideo::run 15 %d\n", write_result);

	if(write_result)
	{
		if(!record_thread->monitor)
		{
			ErrorBox error_box(PROGRAM_NAME ": Error",
				mwindow->gui->get_abs_cursor_x(),
				mwindow->gui->get_abs_cursor_y());
			error_box.create_objects("No space left on disk.");
			error_box.run_window();
			batch_done = 1;
		}
	}

	cleanup_recording();
//printf("RecordVideo::run 16 %p %d\n", this, write_result);
}

void RecordVideo::read_buffer()
{
	grab_result = record->vdevice->read_buffer(capture_frame);
	if(!strncmp(record->default_asset->vcodec, QUICKTIME_MJPA, 4) &&
		record->vdevice->is_compressed())
	{
		unsigned char *data = capture_frame->get_data();
		long size = capture_frame->get_compressed_size();
		long allocation = capture_frame->get_compressed_allocated();

		if(data)
		{
			long field2_offset;
			mjpeg_insert_quicktime_markers(&data, 
				&size, 
				&allocation,
				2,
				&field2_offset);
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
		record->vdevice->is_compressed());
	frame_ptr = record->file->get_video_buffer();
	record->get_current_batch()->current_frame = 0;
	record->get_current_batch()->current_sample = 0;
	record->get_current_batch()->session_frames = 0;
	record->get_current_batch()->session_samples = 0;
	gui->update_position(0, record->current_display_length());
}

int RecordVideo::unhang_thread()
{
printf("RecordVideo::unhang_thread\n");
	Thread::end();
}


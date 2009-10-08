
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

#ifndef RECORDVIDEO_H
#define RECORDVIDEO_H

#include "condition.inc"
#include "file.inc"
#include "mutex.inc"
#include "mwindow.inc"
#include "record.inc"
#include "recordgui.inc"
#include "thread.h"
#include "videodevice.inc"

// Default behavior is to read frames and flash to display.
// Optionally writes to disk.

class RecordVideo : public Thread
{
public:
	RecordVideo(MWindow *mwindow,
		Record *record, 
		RecordThread *record_thread);
	~RecordVideo();

	void reset_parameters();
	void run();
// Do all initialization
	int arm_recording();
// Trigger the loop to start
	void start_recording();
	int stop_recording();
	int pause_recording();
	int resume_recording();
	int wait_for_completion();     // For recording to a file.
	int set_parameters(File *file,
							RecordGUI *gui,
							int buffer_size,    // number of frames to write to disk at a time
							int realtime,
							int frames);
	void write_buffer(int skip_new = 0);
	void start_file_thread();
	int unhang_thread();
	void rewind_file();
	void finish_timed();
	void finish_loop();
	void get_capture_frame();
	void read_buffer();

	MWindow *mwindow;
	Record *record;

	RecordThread *record_thread;
	RecordGUI *gui;
	int single_frame;
	int buffer_size;    // number of frames to write to disk at a time
	int64_t buffer_position;   // Position in output buffer being captured to
	VFrame *capture_frame;   // Output frame for preview mode
	Timer delayer;
// result of every disk write
	int write_result;   
// result of every frame grab
	int grab_result;  
// Capture frame
	VFrame ***frame_ptr;
	int64_t current_sample;   // Sample in time of the start of the capture
	int64_t next_sample;      // Sample of next frame
	int64_t total_dropped_frames;  // Total number of frames behind
	int64_t dropped_frames;  // Number of frames currently behind
	int64_t last_dropped_frames;  // Number of dropped frames during the last calculation
	int64_t delay;
// Frame start of this recording in the file
	int64_t record_start;     
// Want one thread to dictate the other during shared device recording.
// Done with batch
	int batch_done;


	int is_recording;
	int is_paused;
	Mutex *unhang_lock;
	Condition *trigger_lock;
private:
	int cleanup_recording();
};


#endif

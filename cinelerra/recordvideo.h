#ifndef RECORDVIDEO_H
#define RECORDVIDEO_H

#include "file.inc"
#include "mutex.h"
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
	long buffer_position;   // Position in output buffer being captured to
	VFrame *capture_frame;   // Output frame for preview mode
	Timer delayer;
// result of every disk write
	int write_result;   
// result of every frame grab
	int grab_result;  
// Capture frame
	VFrame ***frame_ptr;
	long current_sample;   // Sample in time of the start of the capture
	long next_sample;      // Sample of next frame
	long total_dropped_frames;  // Total number of frames behind
	long dropped_frames;  // Number of frames currently behind
	long last_dropped_frames;  // Number of dropped frames during the last calculation
	long delay;
// Frame start of this recording in the file
	long record_start;     
// Want one thread to dictate the other during shared device recording.
// Done with batch
	int batch_done;


	int is_recording;
	int is_paused;
	Mutex unhang_lock;
	Mutex trigger_lock;
private:
	int cleanup_recording();
};


#endif

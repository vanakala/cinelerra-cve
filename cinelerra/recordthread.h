#ifndef RECORDTHREAD_H
#define RECORDTHREAD_H

#include "drivesync.inc"
#include "file.inc"
#include "record.inc"
#include "recordaudio.inc"
#include "recordvideo.inc"
#include "thread.h"
#include "timer.inc"

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
	long sync_position();
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
	Mutex pause_lock, startup_lock, completion_lock, loop_lock;
// Override the operating system
	DriveSync *drivesync;
};


#endif

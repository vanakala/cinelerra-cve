#ifndef RECORDAUDIO_H
#define RECORDAUDIO_H

#include "audiodevice.inc"
#include "guicast.h"
#include "file.inc"
#include "mutex.h"
#include "mwindow.inc"
#include "record.inc"
#include "recordgui.inc"
#include "recordthread.inc"
#include "thread.h"


class RecordAudio : public Thread
{
public:
	RecordAudio(MWindow *mwindow,
		Record *record, 
		RecordThread *record_thread);
	~RecordAudio();
	
	void run();
// start saving audio data to file
	int arm_recording();
	void start_recording();
// Called by record thread if audio is the only thing.
// Called by video recording if video and audio are being recorded.
	int stop_recording();
	void rewind_file();
	void finish_loop();
	void finish_timed();
	void reset_parameters();

// seek to a new location in the file 
	int set_position(int64_t position);     
	int64_t sync_position();

	void write_buffer(int skip_new = 0);           // write the buffer
// Want one thread to dictate the other during shared device recording.
// Done with batch
	int batch_done;


private:
	MWindow *mwindow;
	Record *record;
	RecordThread *record_thread;
	double *max;
	int *over;
	double **input;
	RecordGUI *gui;
	int64_t buffer_size, fragment_size, fragment_position;
	int record_channels;
	Mutex timer_lock;
	Mutex trigger_lock;
	Timer timer;
	int write_result, grab_result;
};



#endif

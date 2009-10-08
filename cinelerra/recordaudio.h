
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

#ifndef RECORDAUDIO_H
#define RECORDAUDIO_H

#include "audiodevice.inc"
#include "condition.inc"
#include "guicast.h"
#include "file.inc"
#include "mutex.inc"
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
	int buffer_size, fragment_size;
	int64_t fragment_position;
	int record_channels;
	Mutex *timer_lock;
	Condition *trigger_lock;
	Timer timer;
	int write_result, grab_result;
};



#endif

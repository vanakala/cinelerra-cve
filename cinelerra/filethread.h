// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FILETHREAD_H
#define FILETHREAD_H

#include "aframe.inc"
#include "condition.inc"
#include "datatype.h"
#include "file.inc"
#include "mutex.inc"
#include "thread.h"
#include "vframe.inc"


// This allows the file hander to write in the background without 
// blocking the write commands.
// Used for recording.

class FileThread : public Thread
{
public:
	FileThread(File *file, int options);
	~FileThread();

// ============================== writing section ==============================
// Allocate the buffers and start loop for writing.
	void start_writing(int buffer_size, 
			int color_model, 
			int ring_buffers);
	void stop_writing();

// write data into next available buffer
	int write_buffer(int size);
// get pointer to next buffer to be written and lock it
	AFrame** get_audio_buffer();
// get pointer to next frame to be written and lock it
	VFrame*** get_video_buffer();

	void run();
	void swap_buffer();

	AFrame ***audio_buffer;
// (VFrame*)(VFrame array *)(Track *)[ring buffer]
	VFrame ****video_buffer;
	int *output_size;  // Number of frames or samples to write
	Condition **output_lock, **input_lock;
// Lock access to the file to allow it to be changed without stopping the loop
	Mutex *file_lock;
	int current_buffer;
	int local_buffer;
	int *last_buffer;  // last_buffer[ring buffer]
	int return_value;
	int do_audio;
	int do_video;
	File *file;
	int ring_buffers;
	int buffer_size;    // Frames or samples per ring buffer

// Mode of operation
	int is_writing;
	int done;

// Duration of clip already decoded
	ptstime start_pts;
	ptstime end_pts;

// Layer of the frame
	int layer;
};

#endif

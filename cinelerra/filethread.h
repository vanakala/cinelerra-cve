
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

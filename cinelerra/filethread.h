
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

#include "condition.inc"
#include "file.inc"
#include "mutex.inc"
#include "thread.h"
#include "vframe.inc"


// This allows the file hander to write in the background without 
// blocking the write commands.
// Used for recording.


// Container for read frames
class FileThreadFrame
{
public:
	FileThreadFrame();
	~FileThreadFrame();

// Frame position in native framerate
	int64_t position;
	int layer;
	VFrame *frame;
};

class FileThread : public Thread
{
public:
	FileThread(File *file, int do_audio, int do_video);
	~FileThread();

	void create_objects(File *file, 
		int do_audio, 
		int do_video);
	void delete_objects();
	void reset();


// ============================== writing section ==============================
	int start_writing();
// Allocate the buffers and start loop for writing.
// compressed - if 1 write_compressed_frames is called in the file
//            - if 0 write_frames is called
	int start_writing(long buffer_size, 
			int color_model, 
			int ring_buffers, 
			int compressed);
	int stop_writing();




// ================================ reading section ============================
// Allocate buffers and start loop for reading
	int start_reading();
	int stop_reading();

	int read_frame(VFrame *frame);
// Set native framerate.
// Called by File::set_video_position.
	int set_video_position(int64_t position);
	int set_layer(int layer);
	int read_buffer();
	int64_t get_memory_usage();

// write data into next available buffer
	int write_buffer(long size);
// get pointer to next buffer to be written and lock it
	double** get_audio_buffer();     
// get pointer to next frame to be written and lock it
	VFrame*** get_video_buffer();     

	void run();
	int swap_buffer();

	double ***audio_buffer;
// (VFrame*)(VFrame array *)(Track *)[ring buffer]
	VFrame ****video_buffer;      
	long *output_size;  // Number of frames or samples to write
// Not used
	int *is_compressed; // Whether to use the compressed data in the frame
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
// Color model of frames
	int color_model;
// Whether to use the compressed data in the frame
	int compressed;

// Mode of operation
	int is_reading;
	int is_writing;
	int done;

// For the reading mode, the thread reads continuously from the given
// point until stopped.
// Maximum frames to preload
#define MAX_READ_FRAMES 4
// Total number of frames preloaded
	int total_frames;
// Allocated frames
	FileThreadFrame *read_frames[MAX_READ_FRAMES];
// If the seeking pattern isn't optimal for asynchronous reading, this is
// set to 1 to stop reading.
	int disable_read;
// Thread waits on this if the maximum frames have been read.
	Condition *read_wait_lock;
// read_frame waits on this if the thread is running.
	Condition *user_wait_lock;
// Lock access to read_frames
	Mutex *frame_lock;
// Position of first frame in read_frames.
// Set by set_video_position and read_frame only.
// Position is in native framerate.
	int64_t start_position;
// Position to read next frame from
	int64_t read_position;
// Last layer a frame was read from
	int layer;
};



#endif

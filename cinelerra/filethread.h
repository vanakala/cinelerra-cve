#ifndef FILETHREAD_H
#define FILETHREAD_H

#include "condition.inc"
#include "file.inc"
#include "mutex.inc"
#include "thread.h"
#include "vframe.inc"


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
	int start_writing();
// Allocate the buffers and start loop.
// compressed - if 1 write_compressed_frames is called in the file
//            - if 0 write_frames is called
	int start_writing(long buffer_size, 
			int color_model, 
			int ring_buffers, 
			int compressed);
	int stop_writing();
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
};



#endif

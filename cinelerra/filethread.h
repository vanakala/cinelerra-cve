#ifndef FILETHREAD_H
#define FILETHREAD_H

#include "file.inc"
#include "mutex.h"
#include "thread.h"
#include "vframe.inc"


class FileThread : public Thread
{
public:
	FileThread(File *file, int do_audio, int do_video);
	FileThread(File *file, 
		int do_audio, 
		int do_video,
		long buffer_size, 
		int color_model, 
		int ring_buffers, 
		int compressed);
	~FileThread();

	void create_objects(File *file, 
		int do_audio, 
		int do_video,
		long buffer_size, 
		int color_model, 
		int ring_buffers, 
		int compressed);
	int start_writing();
// Allocate the buffers and start loop
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

	double **audio_buffer[RING_BUFFERS];
// (VFrame*)(VFrame array *)(Track *)[ring buffer]
	VFrame ***video_buffer[RING_BUFFERS];      
	long output_size[RING_BUFFERS];  // Number of frames or samples to write
	int is_compressed[RING_BUFFERS]; // Whether to use the compressed data in the frame
	Mutex output_lock[RING_BUFFERS], input_lock[RING_BUFFERS];
// Lock access to the file to allow it to be changed without stopping the loop
	Mutex file_lock;
	int current_buffer;
	int local_buffer;
	int last_buffer[RING_BUFFERS];
	int return_value;
	int do_audio;
	int do_video;
	File *file;
	int ring_buffers;
	int buffer_size;    // Frames or samples per ring buffer
// Color model of frames
	int color_model;
	int compressed;
};



#endif

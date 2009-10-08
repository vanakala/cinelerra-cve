
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

#ifndef DEVICE1394INPUT_H
#define DEVICE1394INPUT_H



#ifdef HAVE_FIREWIRE

#include "condition.inc"
#include "libdv.h"
#include "dv1394.h"
#include "mutex.inc"
#include <libraw1394/raw1394.h>
#include "thread.h"
#include "vframe.inc"

// Common 1394 input for audio and video

// Extracts video and audio from the single DV stream
class Device1394Input : public Thread
{
public:
	Device1394Input();
	~Device1394Input();

	int open(const char *path,
		int port,
		int channel,
		int length,
		int channels,
		int samplerate,
		int bits,
		int w,
		int h);
	void run();
	void increment_counter(int *counter);
	void decrement_counter(int *counter);

// Read a video frame with timed blocking

	int read_video(VFrame *data);


// Read audio with timed blocking

	int read_audio(char *data, int samples);

// Storage of all frames
	char **buffer;
	int *buffer_valid;
	int buffer_size;
	int total_buffers;
	int current_inbuffer;

// For extracting audio
	dv_t *decoder;

// Storage of audio data
	char *audio_buffer;
	int audio_samples;

// number of next video buffer to read
	int current_outbuffer;
	unsigned char *input_buffer;

	Mutex *buffer_lock;
	Condition *video_lock;
	Condition *audio_lock;
	int done;

	int fd;
	int channel;
	int length;
	int channels;
	int samplerate;
	int bits;
	int w;
	int h;
	int is_pal;
};





#endif



#endif

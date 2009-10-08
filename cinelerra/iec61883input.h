
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

#ifndef IEC61883INPUT_H
#define IEC61883INPUT_H



#ifdef HAVE_FIREWIRE

#include "condition.inc"
#include <libiec61883/iec61883.h>
#include "libdv.h"
#include "mutex.inc"
#include "thread.h"
#include "vframe.inc"

// Common 1394 input for audio and video

// Extracts video and audio from the single DV stream
class IEC61883Input : public Thread
{
public:
	IEC61883Input();
	~IEC61883Input();

	int open(int port,
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


	int write_frame(unsigned char *data, int len, int complete);

	raw1394handle_t handle;
	iec61883_dv_fb_t frame;
	int fd;









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

	int port;
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

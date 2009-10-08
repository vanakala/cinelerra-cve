
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

#ifndef IEC61883OUTPUT_H
#define IEC61883OUTPUT_H



#ifdef HAVE_FIREWIRE

#include "audiodevice.inc"
#include "condition.inc"
#include <libiec61883/iec61883.h>
#include "libdv.h"
#include "mutex.inc"
#include "thread.h"
#include "vframe.inc"
#include "videodevice.inc"

// Common 1394 output for audio and video

// This runs continuously to keep the VTR warm.
// Takes encoded DV frames and PCM audio.
class IEC61883Output : public Thread
{
public:
	IEC61883Output(VideoDevice *vdevice);
	IEC61883Output(AudioDevice *adevice);
	~IEC61883Output();

	void reset();
	int open(int port,
		int channel,
		int length,
		int channels, 
		int bits, 
		int samplerate,
		int syt);
	void start();
	void run();


// Write frame with timed blocking.

	void write_frame(VFrame *input);


// Write audio with timed blocking.

	void write_samples(char *data, int samples);
	long get_audio_position();
	void interrupt();
	void flush();



	void increment_counter(int *counter);
	void decrement_counter(int *counter);
	int read_frame(unsigned char *data, int n, unsigned int dropped);


	char **buffer;
	int *buffer_size;
	int *buffer_valid;

	int total_buffers;
	int current_inbuffer;
	int current_outbuffer;

	char *audio_buffer;
	int audio_samples;
// Encoder for audio frames
	dv_t *encoder;

	Mutex *buffer_lock;
// Block while waiting for the first buffer to be allocated
	Condition *start_lock;
	Mutex *position_lock;

// Provide timed blocking for writing routines.

	Condition *video_lock;
	Condition *audio_lock;
	int done;


// Output
	int fd;
	raw1394handle_t handle;
	iec61883_dv_t frame;
// Must break up each frame into 480 byte chunks again.
 	char *out_buffer;
  	int out_size;
	int out_position;


	
	
	
	VFrame *temp_frame, *temp_frame2;
// Encoder for making DV frames
	dv_t *audio_encoder;
	dv_t *video_encoder;
	unsigned char *output_buffer;
	int output_number;
    unsigned int packet_sizes[321];
    unsigned char  continuity_counter;
    int unused_buffers;
	int avc_id;
	int channels;
	int samplerate;
	int bits;
	int syt;
	long audio_position;
	int interrupted;
	int have_video;
	int is_pal;
	VideoDevice *vdevice;
	AudioDevice *adevice;



};




#endif





#endif

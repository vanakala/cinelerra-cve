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

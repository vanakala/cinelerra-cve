#ifndef DEVICE1394INPUT_H
#define DEVICE1394INPUT_H



#ifdef HAVE_FIREWIRE

#include "condition.inc"
#include "libdv.h"
#include "mutex.inc"
#include "raw1394.h"
#include "thread.h"
#include "vframe.inc"

// Common 1394 output for audio and video

// Extracts video and audio from the single DV stream
class Device1394Input : public Thread
{
public:
	Device1394Input();
	~Device1394Input();

	int Device1394Input::open(int port,
		int channel,
		int length,
		int channels,
		int samplerate,
		int bits);
	void run();
	void increment_counter(int *counter);
	void decrement_counter(int *counter);

	static int dv_iso_handler(raw1394handle_t handle, 
		int channel, 
		size_t length,
		quadlet_t *data);
	static bus_reset_handler_t dv_reset_handler(raw1394handle_t handle, 
		unsigned int generation);

// Read a video frame with timed blocking

	int read_video(VFrame *data);


// Read audio with timed blocking

	int read_audio(char *data, int samples);

// Storage of currently downloading frame
	char *temp;
	int bytes_read;

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

	Mutex *buffer_lock;
	Condition *video_lock;
	Condition *audio_lock;
	int done;

	raw1394handle_t handle;
	int channel;
	int length;
	int channels;
	int samplerate;
	int bits;
};





#endif



#endif

#ifndef IEC61883INPUT_H
#define IEC61883INPUT_H



#ifdef HAVE_FIREWIRE

#include "condition.inc"
#include "iec61883.h"
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

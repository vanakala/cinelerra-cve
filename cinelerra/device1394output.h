#ifndef DEVICE1394OUTPUT_H
#define DEVICE1394OUTPUT_H



#ifdef HAVE_FIREWIRE

#include "audiodevice.inc"
#include "condition.inc"
#include "libdv.h"
#include "libdv/dv1394.h"
#include "ieee1394-ioctl.h"
#include "mutex.inc"
#include "thread.h"
#include "vframe.inc"
#include "video1394.h"
#include "videodevice.inc"

// Common 1394 output for audio and video

// This runs continuously to keep the VTR warm.
// Takes encoded DV frames and PCM audio.  Does the 1394 encryption on the fly.
class Device1394Output : public Thread
{
public:
	Device1394Output(VideoDevice *vdevice);
	Device1394Output(AudioDevice *adevice);
	~Device1394Output();

	void reset();
	int open(char *path,
		int port,
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

// This object is shared between audio and video.  Return what the driver is
// based on whether vdevice or adevice exists.
	int get_dv1394();

// Set IOCTL numbers based on kernel version
	void set_ioctls();

	void encrypt(unsigned char *output, 
		unsigned char *data, 
		int data_size);


	void increment_counter(int *counter);
	void decrement_counter(int *counter);


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
 	struct dv1394_status status;


// Output
	int output_fd;
	struct video1394_mmap output_mmap;
	struct video1394_queue_variable output_queue;
//	raw1394handle_t avc_handle;
	VFrame *temp_frame, *temp_frame2;
// Encoder for making DV frames
	dv_t *audio_encoder;
	dv_t *video_encoder;
	unsigned int cip_n, cip_d;
    unsigned int cip_counter;
	unsigned char f50_60;
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

	// IOCTL # variables
	      // dv1394
   int dv1394_init;
   int dv1394_shutdown;
   int dv1394_submit_frames;
   int dv1394_wait_frames;
   int dv1394_receive_frames;
   int dv1394_start_receive;
   int dv1394_get_status;

   // video1394
   int video1394_listen_channel;
   int video1394_unlisten_channel;
   int video1394_listen_queue_buffer;
   int video1394_listen_wait_buffer;
   int video1394_talk_channel;
   int video1394_untalk_channel;
   int video1394_talk_queue_buffer;
   int video1394_talk_wait_buffer;
   int video1394_listen_poll_buffer;


};




#endif





#endif

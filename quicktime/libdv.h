#ifndef LIBDV_H
#define LIBDV_H

#ifdef __cplusplus
extern "C" {
#endif

// Buffer sizes
#define DV_NTSC_SIZE 120000
#define DV_PAL_SIZE 144000

// Norms
#define DV_NTSC 0
#define DV_PAL 1

#ifndef u_int64_t
#define u_int64_t unsigned long long
#endif

#include "libdv/dv.h"
#include <pthread.h>
#include <sys/time.h>

#ifdef HAVE_FIREWIRE
#include "raw1394.h"

typedef struct
{
	raw1394handle_t handle;
	int done;
	unsigned char **frame_buffer;
	long bytes_read;
	long frame_size;
	int output_frame, input_frame;
	int frames;
	int port;
	int channel;

	int crash;
	int still_alive;
	int interrupted;
	int capturing;
	int frame_locked;
	struct timeval delay;
	pthread_t keepalive_tid;

	pthread_t tid;
	pthread_mutex_t *input_lock, *output_lock;
} dv_grabber_t;

typedef struct
{
	raw1394handle_t handle;
	int port;
	int channel;
} dv_playback_t;

#endif

typedef struct
{
	dv_decoder_t *decoder;
	short *temp_audio[4];
	unsigned char *temp_video;
	int use_mmx;
} dv_t;


// ================================== The frame decoder
dv_t* dv_new();
int dv_delete(dv_t* dv);

// Decode a video frame from the data and return nonzero if failure
int dv_read_video(dv_t *dv, 
		unsigned char **output_rows, 
		unsigned char *data, 
		long bytes,
		int color_model);
// Decode audio from the data and return the number of samples decoded.
int dv_read_audio(dv_t *dv, 
		unsigned char *samples,
		unsigned char *data,
		long size);

void dv_write_video(dv_t *dv,
		unsigned char *data,
		unsigned char **input_rows,
		int color_model,
		int norm);

void dv_write_audio(dv_t *dv,
		unsigned char *data,
		double *input_samples,
		int channels,
		int norm);



#ifdef HAVE_FIREWIRE

// ================================== The Firewire grabber
dv_grabber_t *dv_grabber_new();
int dv_grabber_delete(dv_grabber_t *dv);
// Spawn a grabber in the background.  The grabber buffers frames number of frames.
int dv_start_grabbing(dv_grabber_t *grabber, int port, int channel, int buffers);
int dv_stop_grabbing(dv_grabber_t* grabber);
int dv_grab_frame(dv_grabber_t* grabber, unsigned char **frame, long *size);
int dv_unlock_frame(dv_grabber_t* grabber);
int dv_grabber_crashed(dv_grabber_t* grabber);
int dv_interrupt_grabber(dv_grabber_t* grabber);

#endif // HAVE_FIREWIRE

#ifdef __cplusplus
}
#endif

#endif

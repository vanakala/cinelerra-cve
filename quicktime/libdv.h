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

typedef struct
{
	dv_decoder_t *decoder;
	dv_encoder_t *encoder;
	short *temp_audio[4];
	unsigned char *temp_video;
	int use_mmx;
	int audio_frames;
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
		long size,
		int channels,
		int bits);

void dv_write_video(dv_t *dv,
		unsigned char *data,
		unsigned char **input_rows,
		int color_model,
		int norm);

// Write audio into frame after video is encoded.
// Returns the number of samples put in frame.
int dv_write_audio(dv_t *dv,
		unsigned char *data,
		unsigned char *input_samples,
		int max_samples,
		int channels,
		int bits,
		int rate,
		int norm);


#ifdef __cplusplus
}
#endif

#endif

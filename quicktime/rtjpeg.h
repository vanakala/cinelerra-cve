#ifndef QUICKTIME_RTJPEG_H
#define QUICKTIME_RTJPEG_H

#include "pthread.h"
#include "rtjpeg_core.h"

typedef struct
{
	int quality;      /* 0 - 100 quality */
	char *output_buffer;    /* Buffer for RTJPEG data */
	unsigned char *yuv_frame;  /* Buffer for YUV output */
	long image_size;     /* Size of image stored in buffer */
	int buffer_size;    /* Allocated size of input buffer */
	rtjpeg_t compress_struct;
	rtjpeg_t decompress_struct;
} quicktime_rtjpeg_codec_t;

#endif

#ifndef QUICKTIME_JPEG_H
#define QUICKTIME_JPEG_H

#include "libmjpeg.h"

// Jpeg types
#define JPEG_PROGRESSIVE 0
#define JPEG_MJPA 1
#define JPEG_MJPB 2

typedef struct
{
	unsigned char *buffer;
	long buffer_allocated;
	long buffer_size;
	mjpeg_t *mjpeg;
	int jpeg_type;
	unsigned char *temp_video;
} quicktime_jpeg_codec_t;

#endif

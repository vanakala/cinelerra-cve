#ifndef QUICKTIME_JPEG_H
#define QUICKTIME_JPEG_H

#include "fastjpg.h"

/* Too damn slow. */

typedef struct
{
	int quality;
	int use_float;
	quicktime_jpeg_t fastjpg;
	char *chunk_buffer;
	long chunk_buffer_len;
} quicktime_jpeg_codec_t;

#endif

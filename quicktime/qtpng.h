#ifndef QUICKTIME_PNG_H
#define QUICKTIME_PNG_H

#include <png.h>
#include "quicktime.h"

typedef struct
{
	int compression_level;
	unsigned char *buffer;
// Read position
	long buffer_position;
// Frame size
	long buffer_size;
// Buffer allocation
	long buffer_allocated;
	unsigned char *temp_frame;
} quicktime_png_codec_t;

#endif

#ifndef QUICKTIME_WMX1_H
#define QUICKTIME_WMX1_H

#define WMX_CHUNK_FRAMES 50

#include "yuv.h"

typedef struct
{
	quicktime_yuv_t yuv_tables;
	unsigned char *frame_cache[WMX_CHUNK_FRAMES];    /* Frames that are similar enough to pack. */
	unsigned char *key_frame;      /* Packed frames for writing. */
	long keyframe_position;
	int quality;
	int use_float;
/* The YUV4 codec requires a bytes per line that is a multiple of 4 */
	int bytes_per_line;
/* Actual rows encoded in the yuv4 format */
	int rows;

	int frames_per_chunk;
	int threshold;   /* How different a pixel must be for it to be called different. */
} quicktime_wmx1_codec_t;

#endif

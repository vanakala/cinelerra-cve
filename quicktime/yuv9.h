#ifndef QUICKTIME_YUV9_H
#define QUICKTIME_YUV9_H

#include "yuv.h"

typedef struct
{
	quicktime_yuv_t *yuv;
	unsigned char *work_buffer;

/* The YUV9 codec requires a bytes per line that is a multiple of 16 */
	int bytes_per_line;
/* Actual rows encoded in the yuv4 format */
	int rows;
} quicktime_yuv4_codec_t;


/* Now storing data as rows of UVYYYYUVYYYY */

#endif

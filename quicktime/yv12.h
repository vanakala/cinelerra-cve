#ifndef QUICKTIME_YUV2_H
#define QUICKTIME_YUV2_H

#include "colormodels.h"
#include "quicktime.h"

typedef struct
{
	cmodel_yuv_t yuv_table;
	int coded_w, coded_h;
	unsigned char *work_buffer;
} quicktime_yv12_codec_t;

#endif

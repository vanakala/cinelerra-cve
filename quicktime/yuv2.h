#ifndef QUICKTIME_YUV2_H
#define QUICKTIME_YUV2_H

#include "quicktime.h"

typedef struct
{
	int use_float;
	long rtoy_tab[256], gtoy_tab[256], btoy_tab[256];
	long rtou_tab[256], gtou_tab[256], btou_tab[256];
	long rtov_tab[256], gtov_tab[256], btov_tab[256];

	long vtor_tab[256], vtog_tab[256];
	long utog_tab[256], utob_tab[256];
	long *vtor, *vtog, *utog, *utob;
	
	unsigned char *work_buffer;
	int coded_w, coded_h;

/* The YUV2 codec requires a bytes per line that is a multiple of 4 */
	int bytes_per_line;
} quicktime_yuv2_codec_t;

#endif

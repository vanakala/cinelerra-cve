#ifndef QUICKTIME_GRAPHICS_H
#define QUICKTIME_GRAPHICS_H

typedef struct
{
	long rtoy_tab[256], gtoy_tab[256], btoy_tab[256];
	long rtou_tab[256], gtou_tab[256], btou_tab[256];
	long rtov_tab[256], gtov_tab[256], btov_tab[256];

	long vtor_tab[256], vtog_tab[256];
	long utog_tab[256], utob_tab[256];
	long *vtor, *vtog, *utog, *utob;
} quicktime_yuv_t;

typedef struct
{
	int *input_x;
	int *input_y;
	int in_w, in_h, out_w, out_h;
} quicktime_scaletable_t;

#endif

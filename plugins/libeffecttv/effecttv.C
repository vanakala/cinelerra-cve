// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "clip.h"
#include "colormodels.inc"
#include "effecttv.h"
#include "vframe.h"

#include <stdint.h> 
#include <stdio.h>

unsigned int EffectTV::fastrand_val = 0;

EffectTV::EffectTV(int w, int h)
{
	this->w = w;
	this->h = h;
	background = (unsigned char*)new uint16_t[w * h];
	diff = new unsigned char[w * h];
	diff2 = new unsigned char[w * h];
}

EffectTV::~EffectTV()
{
	delete [] background;
	delete [] diff;
	delete [] diff2;
}

void EffectTV::image_set_threshold_y(int threshold)
{
	y_threshold = threshold * 7; /* fake-Y value is timed by 7 */
}

unsigned char* EffectTV::image_bgsubtract_y(unsigned char *input,
	int color_model, int bytes_per_line)
{
	int16_t *q;
	unsigned char *r;
	int R, G, B;
	int v;

	q = (int16_t *)background;
	r = diff;

	switch(color_model)
	{
	case BC_RGBA16161616:
		for(int i = 0; i < h; i++)
		{
			uint16_t *p = (uint16_t*)&input[i * bytes_per_line];

			for(int j = 0; j < w; j++)
			{
				R = p[0] >> (8 - 1);
				G = p[1] >> (8 - 2);
				B = p[2] >> 8;

				v = (R + G + B) - *q;
				*r = ((v + y_threshold) >> 24) | ((y_threshold - v) >> 24);

				p += 4;
				q++;
				r++;
			}
		}
		break;

	case BC_AYUV16161616:
		for(int i = 0; i < h; i++)
		{
			uint16_t *p = (uint16_t*)&input[i * bytes_per_line];

			for(int j = 0; j < w; j++)
			{
				R = G = B = p[1] >> 8;
				R <<= 1;
				G <<= 2;

				v = (R + G + B) - *q;
				*r = ((v + y_threshold) >> 24) | ((y_threshold - v) >> 24);

				p += 4;
				q++;
				r++;
			}
		}
		break;
	}
	return diff;


// The origin of subtraction function is;
// diff(src, dest) = (abs(src - dest) > threshold) ? 0xff : 0;
//
// This functions is transformed to;
// (threshold > (src - dest) > -threshold) ? 0 : 0xff;
//
// (v + threshold)>>24 is 0xff when v is less than -threshold.
// (v - threshold)>>24 is 0xff when v is less than threshold.
// So, ((v + threshold)>>24) | ((threshold - v)>>24) will become 0xff when
// abs(src - dest) > threshold.
}

unsigned char* EffectTV::image_diff_filter(unsigned char *diff)
{
	int x, y;
	unsigned char *src, *dest;
	unsigned int count;
	unsigned int sum1, sum2, sum3;
	int width = w;
	int height = h;

	src = diff;
	dest = diff2 + width + 1;
	for(y = 1; y < height - 1; y++) 
	{
		sum1 = src[0] + src[width] + src[width * 2];
		sum2 = src[1] + src[width + 1] + src[width * 2 + 1];
		src += 2;

		for(x = 1; x < width - 1; x++) 
		{
			sum3 = src[0] + src[width] + src[width * 2];
			count = sum1 + sum2 + sum3;
			sum1 = sum2;
			sum2 = sum3;
			*dest++ = (0xff * 3 - count) >> 24;
			src++;
		}
		dest += 2;
	}
	return diff2;
}

void EffectTV::image_bgset_y(VFrame *frame)
{
	int R, G, B;
	int16_t *q = (int16_t *)background;
	int width = frame->get_w();
	int height = frame->get_h();

	switch(frame->get_color_model())
	{
	case BC_RGBA16161616:
		for(int i = 0; i < height; i++)
		{
			uint16_t *p = (uint16_t*)frame->get_row_ptr(i);

			for(int j = 0; j < width; j++)
			{
				R = p[0] >> (8 - 1);
				G = p[1] >> (8 - 2);
				B = p[2] >> 8;

				*q = (int16_t)(R + G + B);
				p += 4;
				q++;
			}
		}
		break;

	case BC_AYUV16161616:
		for(int i = 0; i < height; i++)
		{
			uint16_t *p = (uint16_t*)frame->get_row_ptr(i);

			for(int j = 0; j < width; j++)
			{
				R = G = B = p[1] >> 8;
				R <<= 1;
				G <<= 2;

				*q = (int16_t)(R + G + B);
				p += 4;
				q++;
			}
		}
		break;
	}
}

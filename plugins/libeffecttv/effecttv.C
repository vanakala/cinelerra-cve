
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "colormodels.h"
#include "effecttv.h"
#include "plugincolors.h"
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
	yuv_init();
	yuv = new YUV;
}

EffectTV::~EffectTV()
{
	delete [] background;
	delete [] diff;
	delete [] diff2;
	delete yuv;
}


int EffectTV::yuv_init()
{
	int i;

	for(i = 0; i < 256; i++)
	{
		YtoRGB[i] =  (int)( 1.164 * (i - 16));
		VtoR[i] = 	 (int)( 1.596 * (i - 128));
		VtoG[i] = 	 (int)(-0.813 * (i - 128));
		UtoG[i] = 	 (int)(-0.391 * (i - 128));
		UtoB[i] = 	 (int)( 2.018 * (i - 128));
		RtoY[i] = 	 (int)( 0.257 * i);
		RtoU[i] = 	 (int)(-0.148 * i);
		RtoV[i] = 	 (int)( 0.439 * i);
		GtoY[i] = 	 (int)( 0.504 * i);
		GtoU[i] = 	 (int)(-0.291 * i);
		GtoV[i] = 	 (int)(-0.368 * i);
		BtoY[i] = 	 (int)( 0.098 * i);
		BtoV[i] = 	 (int)(-0.071 * i);
//printf("EffectTV::yuv_init %d %d %d\n", RtoY[i], GtoY[i], BtoY[i]);
	}
	return 0;
}


#define FRAME_TO_EFFECTTV(type, components) \
{ \
	for(int i = 0; i < width; i++) \
	{ \
		uint32_t *row = tmp + i * width * sizeof(uint32_t); \
		type *input_row = ((type**)frame->get_rows())[i]; \
 \
		for(int j = 0; j < width; j++) \
		{ \
			if(sizeof(type) == 1) \
			{ \
				row[j * sizeof(uint32_t)] = ((uint32_t)input_row[0]) << 16; \
				row[j * sizeof(uint32_t)] |= ((uint32_t)input_row[1]) << 8; \
				row[j * sizeof(uint32_t)] |= input_row[2]; \
			} \
			else \
			{ \
				row[j * sizeof(uint32_t)] = ((uint32_t)input_row[0]) << 8; \
				row[j * sizeof(uint32_t)] |= ((uint32_t)input_row[1]); \
				row[j * sizeof(uint32_t)] |= input_row[2] >> 8; \
			} \
			input_row += components; \
			row++; \
		} \
	} \
}


void EffectTV::frame_to_effecttv(VFrame *frame, uint32_t *tmp)
{
	int width = frame->get_w();
	int height = frame->get_h();

	switch(frame->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			FRAME_TO_EFFECTTV(uint8_t, 3);
			break;
		
		case BC_RGBA8888:
		case BC_YUVA8888:
			FRAME_TO_EFFECTTV(uint8_t, 4);
			break;

		case BC_RGB161616:
		case BC_YUV161616:
			FRAME_TO_EFFECTTV(uint16_t, 3);
			break;
		
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			FRAME_TO_EFFECTTV(uint16_t, 4);
			break;
	}
}



#define EFFECTTV_TO_FRAME(type, components) \
{ \
	for(int i = 0; i < width; i++) \
	{ \
		uint32_t *row = tmp + i * width * sizeof(uint32_t); \
		type *output_row = ((type**)frame->get_rows())[i]; \
 \
		for(int j = 0; j < width; j++) \
		{ \
			if(sizeof(type) == 1) \
			{ \
				output_row[0] = (row[j * sizeof(uint32_t)] & 0xff0000) >> 16; \
				output_row[1] = (row[j * sizeof(uint32_t)] & 0xff00) >> 8; \
				output_row[2] = row[j * sizeof(uint32_t)] & 0xff; \
				if(components == 4) output_row[3] = 0xff; \
			} \
			else \
			{ \
				output_row[0] = (row[j * sizeof(uint32_t)] & 0xff0000) >> 8; \
				output_row[1] = row[j * sizeof(uint32_t)] & 0xff00; \
				output_row[2] = (row[j * sizeof(uint32_t)] & 0xff) << 8; \
				if(components == 4) output_row[3] = 0xffff; \
			} \
			output_row += components; \
			row++; \
		} \
	} \
}


void EffectTV::effecttv_to_frame(VFrame *frame, uint32_t *tmp)
{
	int width = frame->get_w();
	int height = frame->get_h();

	switch(frame->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			EFFECTTV_TO_FRAME(uint8_t, 3);
			break;
		
		case BC_RGBA8888:
		case BC_YUVA8888:
			EFFECTTV_TO_FRAME(uint8_t, 4);
			break;

		case BC_RGB161616:
		case BC_YUV161616:
			EFFECTTV_TO_FRAME(uint16_t, 3);
			break;
		
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			EFFECTTV_TO_FRAME(uint16_t, 4);
			break;
	}
}



void EffectTV::image_set_threshold_y(int threshold)
{
	y_threshold = threshold * 7; /* fake-Y value is timed by 7 */
}



/* Background image is refreshed every frame */
#define IMAGE_BGSUBTRACT_UPDATE_Y(result, \
	input_rows,  \
	type,  \
	components, \
	is_yuv) \
{ \
	int i, j; \
	int R, G, B; \
	type *p; \
	int16_t *q; \
	unsigned char *r; \
	int v; \
 \
	q = (int16_t *)background; \
	r = diff; \
 \
	for(i = 0; i < h; i++)  \
	{ \
		p = (type*)input_rows[j]; \
 \
		for(j = 0; j < w; j++) \
		{ \
			if(is_yuv && sizeof(type) == 2) \
			{ \
				R = G = B = (int)p[0] >> 8; \
				R <<= 1; \
				G <<= 2; \
			} \
			else \
			if(is_yuv && sizeof(type) == 1) \
			{ \
				R = G = B = (int)p[0]; \
				R <<= 1; \
				G <<= 2; \
			} \
			else \
			if(sizeof(type) == 4) \
			{ \
				R = (int)(p[0] * 0x1ff); \
				G = (int)(p[1] * 0x3ff); \
				B = (int)(p[2] * 0xff); \
				CLAMP(R, 0, 0x1ff); \
				CLAMP(G, 0, 0x3ff); \
				CLAMP(B, 0, 0xff); \
			} \
			else \
			if(sizeof(type) == 2) \
			{ \
				R = (int)p[0] >> (8 - 1); \
				G = (int)p[1] >> (8 - 2); \
				B = (int)p[2] >> 8; \
			} \
			else \
			{ \
				R = (int)p[0] << 1; \
				G = (int)p[1] << 2; \
				B = (int)p[2]; \
			} \
 \
			v = (R + G + B) - (int)(*q); \
			*q = (int16_t)(R + G + B); \
			*r = ((v + y_threshold) >> 24) | ((y_threshold - v) >> 24); \
 \
			p += components; \
			q++; \
			r++; \
		} \
	} \
 \
	result = diff; \
}







unsigned char* EffectTV::image_bgsubtract_update_y(unsigned char **input_rows, 
	unsigned char **output_rows, 
	int color_model)
{
	unsigned char *diff;


	switch(color_model)
	{
		case BC_RGB888:
			IMAGE_BGSUBTRACT_UPDATE_Y(diff, 
				input_rows, 
				uint8_t, 
				3,
				0);
			break;
		case BC_YUV888:
			IMAGE_BGSUBTRACT_UPDATE_Y(diff, 
				input_rows, 
				uint8_t, 
				3,
				1);
			break;
		case BC_RGB_FLOAT:
			IMAGE_BGSUBTRACT_UPDATE_Y(diff, 
				input_rows, 
				float, 
				3,
				0);
			break;
		case BC_RGBA8888:
			IMAGE_BGSUBTRACT_UPDATE_Y(diff, 
				input_rows, 
				uint8_t, 
				4,
				0);
			break;
		case BC_RGBA_FLOAT:
			IMAGE_BGSUBTRACT_UPDATE_Y(diff, 
				input_rows, 
				float, 
				4,
				0);
			break;
		case BC_YUVA8888:
			IMAGE_BGSUBTRACT_UPDATE_Y(diff, 
				input_rows, 
				uint8_t, 
				4,
				1);
			break;
		case BC_RGB161616:
			IMAGE_BGSUBTRACT_UPDATE_Y(diff, 
				input_rows, 
				uint16_t, 
				3,
				0);
			break;
		case BC_YUV161616:
			IMAGE_BGSUBTRACT_UPDATE_Y(diff, 
				input_rows, 
				uint16_t, 
				3,
				1);
			break;
		case BC_RGBA16161616:
			IMAGE_BGSUBTRACT_UPDATE_Y(diff, 
				input_rows, 
				uint16_t, 
				4,
				0);
			break;
		case BC_YUVA16161616:
			IMAGE_BGSUBTRACT_UPDATE_Y(diff, 
				input_rows, 
				uint16_t, 
				4,
				1);
			break;
	}
	
	return diff;
}






#define IMAGE_BGSUBTRACT_Y(type, components, is_yuv) \
{ \
	int i, j; \
	int R, G, B; \
	type *p; \
 \
	for(i = 0; i < h; i++) \
	{ \
		p = (type*)input_rows[i]; \
 \
		for(j = 0; j < w; j++) \
		{ \
			if(is_yuv && sizeof(type) == 2) \
			{ \
				R = G = B = (int)p[0] >> 8; \
				R <<= 1; \
				G <<= 2; \
			} \
			else \
			if(is_yuv && sizeof(type) == 1) \
			{ \
				R = G = B = (int)p[0]; \
				R <<= 1; \
				G <<= 2; \
			} \
			else \
			if(sizeof(type) == 4) \
			{ \
				R = (int)(p[0] * 0x1ff); \
				G = (int)(p[1] * 0x3ff); \
				B = (int)(p[2] * 0xff); \
				CLAMP(R, 0, 0x1ff); \
				CLAMP(G, 0, 0x3ff); \
				CLAMP(B, 0, 0xff); \
			} \
			else \
			if(sizeof(type) == 2) \
			{ \
				R = (int)p[0] >> (8 - 1); \
				G = (int)p[1] >> (8 - 2); \
				B = (int)p[2] >> 8; \
			} \
			else \
			{ \
				R = (int)p[0] << 1; \
				G = (int)p[1] << 2; \
				B = (int)p[2]; \
			} \
 \
			v = (R + G + B) - (int)(*q); \
			*r = ((v + y_threshold) >> 24) | ((y_threshold - v) >> 24); \
 \
			p += components; \
			q++; \
			r++; \
		} \
	} \
}


unsigned char* EffectTV::image_bgsubtract_y(unsigned char **input_rows, 
	int color_model)
{
	int16_t *q;
	unsigned char *r;
	int v;

	q = (int16_t *)background;
	r = diff;



	switch(color_model)
	{
		case BC_RGB888:
			IMAGE_BGSUBTRACT_Y(uint8_t, 3, 0);
			break;
		case BC_YUV888:
			IMAGE_BGSUBTRACT_Y(uint8_t, 3, 1);
			break;
		case BC_RGB_FLOAT:
			IMAGE_BGSUBTRACT_Y(float, 3, 0);
			break;
		case BC_RGBA_FLOAT:
			IMAGE_BGSUBTRACT_Y(float, 4, 0);
			break;
		case BC_RGBA8888:
			IMAGE_BGSUBTRACT_Y(uint8_t, 4, 0);
			break;
		case BC_YUVA8888:
			IMAGE_BGSUBTRACT_Y(uint8_t, 4, 1);
			break;
		case BC_RGB161616:
			IMAGE_BGSUBTRACT_Y(uint16_t, 3, 0);
			break;
		case BC_YUV161616:
			IMAGE_BGSUBTRACT_Y(uint16_t, 3, 1);
			break;
		case BC_RGBA16161616:
			IMAGE_BGSUBTRACT_Y(uint16_t, 4, 0);
			break;
		case BC_YUVA16161616:
			IMAGE_BGSUBTRACT_Y(uint16_t, 4, 1);
			break;
	}



	return diff;




/* The origin of subtraction function is;
 * diff(src, dest) = (abs(src - dest) > threshold) ? 0xff : 0;
 *
 * This functions is transformed to;
 * (threshold > (src - dest) > -threshold) ? 0 : 0xff;
 *
 * (v + threshold)>>24 is 0xff when v is less than -threshold.
 * (v - threshold)>>24 is 0xff when v is less than threshold.
 * So, ((v + threshold)>>24) | ((threshold - v)>>24) will become 0xff when
 * abs(src - dest) > threshold.
 */
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






#define IMAGE_BGSET_Y(type, components, is_yuv) \
{ \
	int i, j; \
	int R, G, B; \
	type *p; \
	int16_t *q; \
	int width = frame->get_w(); \
	int height = frame->get_h(); \
 \
	q = (int16_t *)background; \
 \
 \
 \
	for(i = 0; i < height; i++) \
	{ \
		p = (type*)frame->get_rows()[i]; \
 \
		for(j = 0; j < width; j++) \
		{ \
			if(is_yuv && sizeof(type) == 2) \
			{ \
				R = G = B = (int)p[0] >> 8; \
				R <<= 1; \
				G <<= 2; \
			} \
			else \
			if(is_yuv && sizeof(type) == 1) \
			{ \
				R = G = B = (int)p[0]; \
				R <<= 1; \
				G <<= 2; \
			} \
			else \
			if(sizeof(type) == 4) \
			{ \
				R = (int)(p[0] * 0x1ff); \
				G = (int)(p[1] * 0x3ff); \
				B = (int)(p[2] * 0xff); \
				CLAMP(R, 0, 0x1ff); \
				CLAMP(G, 0, 0x3ff); \
				CLAMP(B, 0, 0xff); \
			} \
			else \
			if(sizeof(type) == 2) \
			{ \
				R = (int)p[0] >> (8 - 1); \
				G = (int)p[1] >> (8 - 2); \
				B = (int)p[2] >> 8; \
			} \
			else \
			{ \
				R = (int)p[0] << 1; \
				G = (int)p[1] << 2; \
				B = (int)p[2]; \
			} \
 \
			*q = (int16_t)(R + G + B); \
			p += components; \
 \
 \
			q++; \
		} \
	} \
}





void EffectTV::image_bgset_y(VFrame *frame)
{
	switch(frame->get_color_model())
	{
		case BC_RGB888:
			IMAGE_BGSET_Y(uint8_t, 3, 0);
			break;
		case BC_RGB_FLOAT:
			IMAGE_BGSET_Y(float, 3, 0);
			break;
		case BC_YUV888:
			IMAGE_BGSET_Y(uint8_t, 3, 1);
			break;
		case BC_RGBA8888:
			IMAGE_BGSET_Y(uint8_t, 3, 0);
			break;
		case BC_RGBA_FLOAT:
			IMAGE_BGSET_Y(float, 3, 0);
			break;
		case BC_YUVA8888:
			IMAGE_BGSET_Y(uint8_t, 3, 1);
			break;
		case BC_RGB161616:
			IMAGE_BGSET_Y(uint16_t, 3, 0);
			break;
		case BC_YUV161616:
			IMAGE_BGSET_Y(uint16_t, 3, 1);
			break;
		case BC_RGBA16161616:
			IMAGE_BGSET_Y(uint16_t, 4, 0);
			break;
		case BC_YUVA16161616:
			IMAGE_BGSET_Y(uint16_t, 4, 1);
			break;
	}
}



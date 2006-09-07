/*
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 
 * USA
 */
#include <sys/time.h>
#include <time.h>
#include "cmodel_permutation.h"









// ********************************* YUV101010 -> *****************************

#define READ_YUV101010 \
	uint64_t y, u, v; \
	uint32_t input_i = input[0] | \
		(input[1] << 8) | \
		(input[2] << 16) | \
		(input[3] << 24); \
 \
	y = ((input_i & 0xffc00000) >> 16) | 0x3f; \
	u = ((input_i & 0x3ff000) >> 6) | 0x3f; \
	v = ((input_i & 0xffc) << 4) | 0x3f;






static inline void transfer_YUV101010_to_RGB8(unsigned char *(*output), unsigned char *input)
{
	int r, g, b;

	READ_YUV101010

	y = (y << 8) | (y >> 8);

	YUV_TO_RGB16(y, u, v, r, g, b);

	*(*output)++ = (unsigned char)(((input[0] & 0xc000) >> 8) +
			    			 ((input[1] & 0xe000) >> 10) +
		 	    			 ((input[2] & 0xe000) >> 13));
}

static inline void transfer_YUV101010_to_BGR565(unsigned char *(*output), unsigned char *input)
{
	int r, g, b;

	READ_YUV101010

	y = (y << 8) | (y >> 8);

	YUV_TO_RGB16(y, u, v, r, g, b);

	*(uint16_t*)(*output) = (b & 0xf800) |
			 ((g & 0xfc00) >> 5) |
			 ((r & 0xf800) >> 11);
	(*output) += 2;
}

static inline void transfer_YUV101010_to_RGB565(unsigned char *(*output), unsigned char *input)
{
	int r, g, b;

	READ_YUV101010

	y = (y << 8) | (y >> 8);

	YUV_TO_RGB16(y, u, v, r, g, b);

	*(uint16_t*)(*output) = (r & 0xf800) |
			 ((g & 0xfc00) >> 5) |
			 ((b & 0xf800) >> 11);
	(*output) += 2;
}

static inline void transfer_YUV101010_to_BGR888(unsigned char *(*output), unsigned char *input)
{
	int r, g, b;

	READ_YUV101010

	y = (y << 8) | (y >> 8);

	YUV_TO_RGB16(y, u, v, r, g, b);

	*(*output)++ = b >> 8;
	*(*output)++ = g >> 8;
	*(*output)++ = r >> 8;
}

static inline void transfer_YUV101010_to_BGR8888(unsigned char *(*output), unsigned char *input)
{
	int r, g, b;

	READ_YUV101010

	y = (y << 8) | (y >> 8);

	YUV_TO_RGB16(y, u, v, r, g, b);

	*(*output)++ = b >> 8;
	*(*output)++ = g >> 8;
	*(*output)++ = r >> 8;
	(*output)++;
}

static inline void transfer_YUV101010_to_YUV888(unsigned char *(*output), unsigned char *input)
{
	READ_YUV101010
	 
	*(*output)++ = y >> 8;
	*(*output)++ = u >> 8;
	*(*output)++ = v >> 8;
}

static inline void transfer_YUV101010_to_YUVA8888(unsigned char *(*output), unsigned char *input)
{
	READ_YUV101010
	 
	*(*output)++ = y >> 8;
	*(*output)++ = u >> 8;
	*(*output)++ = v >> 8;
	*(*output)++ = 0xff;
}

static inline void transfer_YUV101010_to_YUV161616(uint16_t *(*output), unsigned char *input)
{
	READ_YUV101010
	 
	*(*output)++ = y;
	*(*output)++ = u;
	*(*output)++ = v;
}

static inline void transfer_YUV101010_to_YUVA16161616(uint16_t *(*output), unsigned char *input)
{
	READ_YUV101010
	 
	*(*output)++ = y;
	*(*output)++ = u;
	*(*output)++ = v;
	*(*output)++ = 0xffff;
}

static inline void transfer_YUV101010_to_RGB888(unsigned char *(*output), unsigned char *input)
{
	int r, g, b;

	READ_YUV101010

	y = (y << 8) | (y >> 8);

	YUV_TO_RGB16(y, u, v, r, g, b);

	*(*output)++ = r >> 8;
	*(*output)++ = g >> 8;
	*(*output)++ = b >> 8;
}

static inline void transfer_YUV101010_to_RGBA8888(unsigned char *(*output), unsigned char *input)
{
	int r, g, b;

	READ_YUV101010

	y = (y << 8) | (y >> 8);

	YUV_TO_RGB16(y, u, v, r, g, b);

	*(*output)++ = r >> 8;
	*(*output)++ = g >> 8;
	*(*output)++ = b >> 8;
	*(*output)++ = 0xff;
}

static inline void transfer_YUV101010_to_RGB161616(uint16_t *(*output), unsigned char *input)
{
	int r, g, b;

	READ_YUV101010

	y = (y << 8) | (y >> 8);

	YUV_TO_RGB16(y, u, v, r, g, b);

	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
}

static inline void transfer_YUV101010_to_RGBA16161616(uint16_t *(*output), unsigned char *input)
{
	int r, g, b;

	READ_YUV101010

	y = (y << 8) | (y >> 8);

	YUV_TO_RGB16(y, u, v, r, g, b);

	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
	*(*output)++ = 0xffff;
}




static inline void transfer_YUV101010_to_RGB_FLOAT(float *(*output), 
	unsigned char *input)
{
	float r, g, b;
	float y_f;

	READ_YUV101010

	y_f = (float)y / 0xffff;

	YUV16_TO_RGB_FLOAT(y_f, u, v, r, g, b);

	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
}

static inline void transfer_YUV101010_to_RGBA_FLOAT(float *(*output), 
	unsigned char *input)
{
	float r, g, b;
	float y_f;

	READ_YUV101010

	y_f = (float)y / 0xffff;

	YUV16_TO_RGB_FLOAT(y_f, u, v, r, g, b);

	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
	*(*output)++ = 1.0;
}



















// ******************************** VYU888 -> *********************************


static inline void transfer_VYU888_to_RGB8(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[2];
	v = input[0];
	YUV_TO_RGB(y, u, v, r, g, b);

	*(*output) = (unsigned char)((r & 0xc0) +
			    			 ((g & 0xe0) >> 2) +
		 	    			 ((b & 0xe0) >> 5));
	(*output)++;
}

static inline void transfer_VYU888_to_BGR565(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[2];
	v = input[0];
	YUV_TO_RGB(y, u, v, r, g, b);
	*(uint16_t*)(*output) = ((b & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((r & 0xf8) >> 3);
	(*output) += 2;
}

static inline void transfer_VYU888_to_RGB565(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[2];
	v = input[0];
	YUV_TO_RGB(y, u, v, r, g, b);
	*(uint16_t*)(*output) = ((r & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((b & 0xf8) >> 3);
	(*output) += 2;
}

static inline void transfer_VYU888_to_BGR888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[2];
	v = input[0];
	YUV_TO_RGB(y, u, v, r, g, b);

	(*output)[2] = r;
	(*output)[1] = g;
	(*output)[0] = b;
	(*output) += 3;
}

static inline void transfer_VYU888_to_BGR8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;

	y = ((int)input[1]) << 16;
	u = input[2];
	v = input[0];
	YUV_TO_RGB(y, u, v, r, g, b);
	(*output)[2] = r;
	(*output)[1] = g;
	(*output)[0] = b;
	(*output) += 4;
}


static inline void transfer_VYU888_to_RGB888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[2];
	v = input[0];
	YUV_TO_RGB(y, u, v, r, g, b);

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output) += 3;
}

static inline void transfer_VYU888_to_RGBA8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[2];
	v = input[0];
	YUV_TO_RGB(y, u, v, r, g, b);

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output)[3] = 0xff;
	(*output) += 4;
}


static inline void transfer_VYU888_to_RGB161616(uint16_t *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = (input[1] << 16) | (input[1] << 8) | input[1];
	u = (input[2] << 8) | input[2];
	v = (input[0] << 8) | input[0];
	YUV_TO_RGB16(y, u, v, r, g, b);

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output) += 3;
}

static inline void transfer_VYU888_to_RGBA16161616(uint16_t *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;

	y = (input[1] << 16) | (input[1] << 8) | input[1];
	u = (input[2] << 8) | input[2];
	v = (input[0] << 8) | input[0];
	YUV_TO_RGB16(y, u, v, r, g, b);

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output)[3] = 0xffff;
	(*output) += 3;
}


static inline void transfer_VYU888_to_RGB_FLOAT(float *(*output), unsigned char *input)
{
	float y;
	int u, v;
	float r, g, b;
	
	v = *input++;
	y = (float)*input++ / 0xff;
	u = *input;
	YUV_TO_FLOAT(y, u, v, r, g, b);

	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
}

static inline void transfer_VYU888_to_RGBA_FLOAT(float *(*output), unsigned char *input)
{
	float y;
	int u, v;
	float r, g, b;
	
	v = *input++;
	y = (float)*input++ / 0xff;
	u = *input;
	YUV_TO_FLOAT(y, u, v, r, g, b);

	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
	*(*output)++ = 1.0;
}


static inline void transfer_VYU888_to_YUV888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[1];
	(*output)[1] = input[2];
	(*output)[2] = input[0];
	(*output) += 3;
}

static inline void transfer_VYU888_to_YUVA8888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[1];
	(*output)[1] = input[2];
	(*output)[2] = input[0];
	(*output)[3] = 0xff;
	(*output) += 4;
}


static inline void transfer_VYU888_to_YUV161616(uint16_t *(*output), unsigned char *input)
{
	(*output)[0] = ((int)input[1]) << 8;
	(*output)[1] = ((int)input[2]) << 8;
	(*output)[2] = ((int)input[0]) << 8;
	(*output) += 3;
}

static inline void transfer_VYU888_to_YUVA16161616(uint16_t *(*output), unsigned char *input)
{
	(*output)[0] = ((int)input[1]) << 8;
	(*output)[1] = ((int)input[2]) << 8;
	(*output)[2] = ((int)input[0]) << 8;
	(*output)[3] = 0xff;
	(*output) += 4;
}












// ******************************** UYVA8888 -> *********************************


static inline void transfer_UYVA8888_to_RGB8(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;

	y = ((int)input[1]) << 16;
	u = input[0];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);

	r = r * input[3] / 0xff;
	g = g * input[3] / 0xff;
	b = b * input[3] / 0xff;
	*(*output) = (unsigned char)((r & 0xc0) +
			    			 ((g & 0xe0) >> 2) +
		 	    			 ((b & 0xe0) >> 5));
	(*output)++;
}

static inline void transfer_UYVA8888_to_BGR565(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[0];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
	r = r * input[3] / 0xff;
	g = g * input[3] / 0xff;
	b = b * input[3] / 0xff;
	*(uint16_t*)(*output) = ((b & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((r & 0xf8) >> 3);
	(*output) += 2;
}

static inline void transfer_UYVA8888_to_RGB565(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[0];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
	r = r * input[3] / 0xff;
	g = g * input[3] / 0xff;
	b = b * input[3] / 0xff;
	*(uint16_t*)(*output) = ((r & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((b & 0xf8) >> 3);
	(*output) += 2;
}

static inline void transfer_UYVA8888_to_BGR888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[0];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
	r = r * input[3] / 0xff;
	g = g * input[3] / 0xff;
	b = b * input[3] / 0xff;

	(*output)[2] = r;
	(*output)[1] = g;
	(*output)[0] = b;
	(*output) += 3;
}

static inline void transfer_UYVA8888_to_BGR8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[0];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
	r = r * input[3] / 0xff;
	g = g * input[3] / 0xff;
	b = b * input[3] / 0xff;

	(*output)[2] = r;
	(*output)[1] = g;
	(*output)[0] = b;
	(*output) += 4;
}


static inline void transfer_UYVA8888_to_RGB888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[0];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
	r = r * input[3] / 0xff;
	g = g * input[3] / 0xff;
	b = b * input[3] / 0xff;

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output) += 3;
}

static inline void transfer_UYVA8888_to_RGBA8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[0];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output)[3] = input[3];
	(*output) += 4;
}


static inline void transfer_UYVA8888_to_RGB161616(uint16_t *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = (input[0] << 8) | input[0];
	v = (input[2] << 8) | input[2];
	YUV_TO_RGB16(y, u, v, r, g, b);
	r = r * input[3] / 0xff;
	g = g * input[3] / 0xff;
	b = b * input[3] / 0xff;

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output) += 3;
}

static inline void transfer_UYVA8888_to_RGBA16161616(uint16_t *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;

	y = ((int)input[1]) << 16;
	u = (input[0] << 8) | input[0];
	v = (input[2] << 8) | input[2];
	YUV_TO_RGB16(y, u, v, r, g, b);

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output)[3] = input[3] << 8;
	(*output) += 4;
}

static inline void transfer_UYVA8888_to_RGB_FLOAT(float *(*output), unsigned char *input)
{
	float y, a;
	int u, v;
	float r, g, b;
	
	u = *input++;
	y = (float)*input++ / 0xff;
	v = *input++;
	a = (float)*input / 0xff;
	YUV_TO_FLOAT(y, u, v, r, g, b);

	r = r * a;
	g = g * a;
	b = b * a;

	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
}

static inline void transfer_UYVA8888_to_RGBA_FLOAT(float *(*output), unsigned char *input)
{
	float y, a;
	int u, v;
	float r, g, b;
	
	u = *input++;
	y = (float)*input++ / 0xff;
	v = *input++;
	a = (float)*input / 0xff;
	YUV_TO_FLOAT(y, u, v, r, g, b);

	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
	*(*output)++ = a;
}


static inline void transfer_UYVA8888_to_YUV888(unsigned char *(*output), unsigned char *input)
{
	int a, anti_a;
	a = input[3];
	anti_a = 0xff - a;

	(*output)[0] = (a * input[1]) / 0xff;
	(*output)[1] = (a * input[0] + anti_a * 0x80) / 0xff;
	(*output)[2] = (a * input[2] + anti_a * 0x80) / 0xff;
	(*output) += 3;
}

static inline void transfer_UYVA8888_to_YUVA8888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[1];
	(*output)[1] = input[0];
	(*output)[2] = input[2];
	(*output)[3] = input[3];
	(*output) += 4;
}


static inline void transfer_UYVA8888_to_YUV161616(uint16_t *(*output), unsigned char *input)
{
	int a, anti_a;
	a = input[3];
	anti_a = 0xff - a;

	(*output)[0] = a * input[1];
	(*output)[1] = a * input[0] + anti_a * 0x80;
	(*output)[2] = a * input[2] + anti_a * 0x80;
	(*output) += 3;
}

static inline void transfer_UYVA8888_to_YUVA16161616(uint16_t *(*output), unsigned char *input)
{
	(*output)[0] = input[1] << 8;
	(*output)[1] = input[0] << 8;
	(*output)[2] = input[2] << 8;
	(*output)[3] = input[3] << 8;
	(*output) += 4;
}










#define TRANSFER_FRAME_DEFAULT(output, \
	input, \
	y_in_offset, \
	u_in_offset, \
	v_in_offset, \
	input_column) \
{ \
	register int i, j; \
 \
	switch(in_colormodel) \
	{ \
		case BC_YUV888: \
			switch(out_colormodel) \
			{ \
				case BC_RGB8: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV888_to_RGB8((output), (input));      \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR565: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV888_to_BGR565((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV888_to_RGB565((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV888_to_BGR888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV888_to_BGR8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV888_to_RGB888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV888_to_RGBA8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_ARGB8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV888_to_ARGB8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV888_to_RGB_FLOAT((float**)(output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV888_to_RGBA_FLOAT((float**)(output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV101010: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV888_to_YUV101010((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV420P: \
					TRANSFER_YUV420P_OUT_HEAD \
					transfer_YUV888_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					TRANSFER_YUV422P_OUT_HEAD \
					transfer_YUV888_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV444P: \
					TRANSFER_YUV444P_OUT_HEAD \
					transfer_YUV888_to_YUV444P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV888_to_YUV422((output), \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV888_to_YUV888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV888_to_YUVA8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_VYU888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV888_to_VYU888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_UYVA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV888_to_UYVA8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
		case BC_YUVA8888: \
			switch(out_colormodel) \
			{ \
				case BC_RGB8: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA8888_to_RGB8((output), (input));      \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR565: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA8888_to_BGR565((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA8888_to_RGB565((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA8888_to_BGR888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA8888_to_BGR8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA8888_to_RGB888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA8888_to_RGBA8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_ARGB8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA8888_to_ARGB8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA8888_to_RGB_FLOAT((float**)(output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA8888_to_RGBA_FLOAT((float**)(output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_VYU888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA8888_to_VYU888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA8888_to_YUVA8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_UYVA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA8888_to_UYVA8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV101010: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA8888_to_YUV101010((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV420P: \
					TRANSFER_YUV420P_OUT_HEAD \
					transfer_YUVA8888_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					TRANSFER_YUV422P_OUT_HEAD \
					transfer_YUVA8888_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV444P: \
					TRANSFER_YUV444P_OUT_HEAD \
					transfer_YUVA8888_to_YUV444P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA8888_to_YUV422((output), \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
		case BC_YUV161616: \
			switch(out_colormodel) \
			{ \
				case BC_RGB8: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV161616_to_RGB8((output), (uint16_t*)(input));      \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR565: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV161616_to_BGR565((output), (uint16_t*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV161616_to_RGB565((output), (uint16_t*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV161616_to_BGR888((output), (uint16_t*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV161616_to_BGR8888((output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV161616_to_RGB888((output), (uint16_t*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV161616_to_RGBA8888((output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_ARGB8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV161616_to_ARGB8888((output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV161616_to_RGB_FLOAT((float**)(output), (uint16_t*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV161616_to_RGBA_FLOAT((float**)(output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV420P: \
					TRANSFER_YUV420P_OUT_HEAD \
					transfer_YUV161616_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(uint16_t*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					TRANSFER_YUV422P_OUT_HEAD \
					transfer_YUV161616_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(uint16_t*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV444P: \
					TRANSFER_YUV444P_OUT_HEAD \
					transfer_YUV161616_to_YUV444P(output_y, \
						output_u, \
						output_v, \
						(uint16_t*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV161616_to_YUV422((output), \
						(uint16_t*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV101010: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV161616_to_YUV101010((output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV161616_to_YUVA8888((output), \
						(uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_VYU888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV161616_to_VYU888((output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_UYVA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV161616_to_UYVA8888((output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV161616: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV161616_to_YUV161616((uint16_t**)(output), \
						(uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
		case BC_YUVA16161616: \
			switch(out_colormodel) \
			{ \
				case BC_RGB8: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA16161616_to_RGB8((output), (uint16_t*)(input));      \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR565: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA16161616_to_BGR565((output), (uint16_t*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA16161616_to_RGB565((output), (uint16_t*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA16161616_to_BGR888((output), (uint16_t*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA16161616_to_BGR8888((output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA16161616_to_RGB888((output), (uint16_t*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA16161616_to_RGBA8888((output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_ARGB8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA16161616_to_ARGB8888((output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA16161616_to_RGB_FLOAT((float**)(output), (uint16_t*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA16161616_to_RGBA_FLOAT((float**)(output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV101010: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA16161616_to_YUV101010((output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_VYU888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA16161616_to_VYU888((output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_UYVA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA16161616_to_UYVA8888((output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA16161616: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA16161616_to_YUVA16161616((uint16_t**)(output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV420P: \
					TRANSFER_YUV420P_OUT_HEAD \
					transfer_YUVA16161616_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(uint16_t*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					TRANSFER_YUV422P_OUT_HEAD \
					transfer_YUVA16161616_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(uint16_t*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV444P: \
					TRANSFER_YUV444P_OUT_HEAD \
					transfer_YUVA16161616_to_YUV444P(output_y, \
						output_u, \
						output_v, \
						(uint16_t*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA16161616_to_YUV422((output), \
						(uint16_t*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
		case BC_YUV101010: \
			switch(out_colormodel) \
			{ \
				case BC_RGB8: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV101010_to_RGB8((output), (input));      \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR565: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV101010_to_BGR565((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV101010_to_RGB565((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV101010_to_BGR888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV101010_to_BGR8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV101010_to_RGB888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV101010_to_RGBA8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV101010_to_YUV888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV101010_to_YUVA8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB161616: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV101010_to_RGB161616((uint16_t**)(output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA16161616: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV101010_to_RGBA16161616((uint16_t**)(output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV101010_to_RGB_FLOAT((float**)(output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV101010_to_RGBA_FLOAT((float**)(output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV161616: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV101010_to_YUV161616((uint16_t**)(output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA16161616: \
					TRANSFER_FRAME_HEAD \
					transfer_YUV101010_to_YUVA16161616((uint16_t**)(output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
		case BC_VYU888: \
			switch(out_colormodel) \
			{ \
				case BC_RGB8: \
					TRANSFER_FRAME_HEAD \
					transfer_VYU888_to_RGB8((output), (input));      \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR565: \
					TRANSFER_FRAME_HEAD \
					transfer_VYU888_to_BGR565((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_FRAME_HEAD \
					transfer_VYU888_to_RGB565((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888: \
					TRANSFER_FRAME_HEAD \
					transfer_VYU888_to_BGR888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_FRAME_HEAD \
					transfer_VYU888_to_BGR8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
					transfer_VYU888_to_RGB888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_VYU888_to_RGBA8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV888: \
					TRANSFER_FRAME_HEAD \
					transfer_VYU888_to_YUV888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_VYU888_to_YUVA8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB161616: \
					TRANSFER_FRAME_HEAD \
					transfer_VYU888_to_RGB161616((uint16_t**)(output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA16161616: \
					TRANSFER_FRAME_HEAD \
					transfer_VYU888_to_RGBA16161616((uint16_t**)(output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_VYU888_to_RGB_FLOAT((float**)(output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_VYU888_to_RGBA_FLOAT((float**)(output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV161616: \
					TRANSFER_FRAME_HEAD \
					transfer_VYU888_to_YUV161616((uint16_t**)(output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA16161616: \
					TRANSFER_FRAME_HEAD \
					transfer_VYU888_to_YUVA16161616((uint16_t**)(output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
		case BC_UYVA8888: \
			switch(out_colormodel) \
			{ \
				case BC_RGB8: \
					TRANSFER_FRAME_HEAD \
					transfer_UYVA8888_to_RGB8((output), (input));      \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR565: \
					TRANSFER_FRAME_HEAD \
					transfer_UYVA8888_to_BGR565((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_FRAME_HEAD \
					transfer_UYVA8888_to_RGB565((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888: \
					TRANSFER_FRAME_HEAD \
					transfer_UYVA8888_to_BGR888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_FRAME_HEAD \
					transfer_UYVA8888_to_BGR8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
					transfer_UYVA8888_to_RGB888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_UYVA8888_to_RGBA8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV888: \
					TRANSFER_FRAME_HEAD \
					transfer_UYVA8888_to_YUV888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_UYVA8888_to_YUVA8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB161616: \
					TRANSFER_FRAME_HEAD \
					transfer_UYVA8888_to_RGB161616((uint16_t**)(output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA16161616: \
					TRANSFER_FRAME_HEAD \
					transfer_UYVA8888_to_RGBA16161616((uint16_t**)(output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_UYVA8888_to_RGB_FLOAT((float**)(output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_UYVA8888_to_RGBA_FLOAT((float**)(output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV161616: \
					TRANSFER_FRAME_HEAD \
					transfer_UYVA8888_to_YUV161616((uint16_t**)(output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA16161616: \
					TRANSFER_FRAME_HEAD \
					transfer_UYVA8888_to_YUVA16161616((uint16_t**)(output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
 		case BC_ARGB8888: \
 		case BC_ABGR8888: \
			switch(out_colormodel) \
			{ \
				case BC_ARGB8888: \
				case BC_ABGR8888: \
					TRANSFER_FRAME_HEAD \
					transfer_ARGB8888_to_ARGB8888((output), (input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_ARGB8888_to_RGBA8888((output), (input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
					transfer_ARGB8888_to_RGB888((output), (input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_FRAME_HEAD \
					transfer_ARGB8888_to_BGR8888((output), (input)); \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
		case BC_RGB888: \
			switch(out_colormodel) \
			{ \
				case BC_RGB8: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_RGB8((output), (input));      \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR565: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_BGR565((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_RGB565((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_BGR888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_RGB888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_RGBA8888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_ARGB8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_ARGB8888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB161616: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_RGB161616((uint16_t**)(output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA16161616: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_RGBA16161616((uint16_t**)(output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_RGB_FLOAT((float**)(output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_RGBA_FLOAT((float**)(output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_ABGR8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_ABGR8888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_BGR8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_YUV888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_YUVA8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV161616: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_YUV161616((uint16_t**)(output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA16161616: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_YUVA16161616((uint16_t**)(output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV101010: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_YUV101010((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV420P: \
					TRANSFER_YUV420P_OUT_HEAD \
					transfer_RGB888_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_YUV422((output), (input), j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					TRANSFER_YUV422P_OUT_HEAD \
					transfer_RGB888_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV444P: \
					TRANSFER_YUV444P_OUT_HEAD \
					transfer_RGB888_to_YUV444P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
		case BC_RGBA8888: \
			switch(out_colormodel) \
			{ \
				case BC_TRANSPARENCY: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA8888_to_TRANSPARENCY((output), (input), &bit_counter); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB8: \
					if(bg_color > 0) \
						TRANSFER_FRAME_HEAD \
						transfer_RGBA8888_to_RGB8bg((output), (input), bg_r, bg_g, bg_b); \
						TRANSFER_FRAME_TAIL \
					else \
						TRANSFER_FRAME_HEAD \
						transfer_RGBA8888_to_RGB8((output), (input)); \
						TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR565: \
					if(bg_color > 0) \
						TRANSFER_FRAME_HEAD \
						transfer_RGBA8888_to_BGR565bg((output), (input), bg_r, bg_g, bg_b); \
						TRANSFER_FRAME_TAIL \
					else \
						TRANSFER_FRAME_HEAD \
						transfer_RGBA8888_to_BGR565((output), (input)); \
						TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					if(bg_color > 0) \
						TRANSFER_FRAME_HEAD \
						transfer_RGBA8888_to_RGB565bg((output), (input), bg_r, bg_g, bg_b); \
						TRANSFER_FRAME_TAIL \
					else \
						TRANSFER_FRAME_HEAD \
						transfer_RGBA8888_to_RGB565((output), (input)); \
						TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888: \
					if(bg_color > 0) \
						TRANSFER_FRAME_HEAD \
						transfer_RGBA8888_to_BGR888bg((output), (input), bg_r, bg_g, bg_b); \
						TRANSFER_FRAME_TAIL \
					else \
						TRANSFER_FRAME_HEAD \
						transfer_RGBA8888_to_BGR888((output), (input)); \
						TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888: \
					if(bg_color > 0) \
						TRANSFER_FRAME_HEAD \
						transfer_RGBA8888_to_RGB888bg((output), (input), bg_r, bg_g, bg_b); \
						TRANSFER_FRAME_TAIL \
					else \
						TRANSFER_FRAME_HEAD \
						transfer_RGBA8888_to_RGB888((output), (input)); \
						TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA8888_to_RGBA8888((output), (input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_ARGB8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA8888_to_ARGB8888((output), (input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB161616: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA8888_to_RGB161616((uint16_t**)(output), (input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA16161616: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA8888_to_RGBA16161616((uint16_t**)(output), (input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA8888_to_RGB_FLOAT((float**)(output), (input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA8888_to_RGBA_FLOAT((float**)(output), (input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					if(bg_color > 0) \
						TRANSFER_FRAME_HEAD \
						transfer_RGBA8888_to_BGR8888bg((output), (input), bg_r, bg_g, bg_b); \
						TRANSFER_FRAME_TAIL \
					else \
						TRANSFER_FRAME_HEAD \
						transfer_RGBA8888_to_BGR8888((output), (input)); \
						TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA8888_to_YUV888((output), (input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA8888_to_YUVA8888((output), (input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV161616: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA8888_to_YUV161616((uint16_t**)(output), (input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA16161616: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA8888_to_YUVA16161616((uint16_t**)(output), (input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV101010: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA8888_to_YUV101010((output), (input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV420P: \
					TRANSFER_YUV420P_OUT_HEAD \
					transfer_RGBA888_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA888_to_YUV422((output), (input), j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					TRANSFER_YUV422P_OUT_HEAD \
					transfer_RGBA888_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV444P: \
					TRANSFER_YUV444P_OUT_HEAD \
					transfer_RGBA888_to_YUV444P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
		case BC_RGB161616: \
			switch(out_colormodel) \
			{ \
				case BC_RGB8: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB161616_to_RGB8((output), (uint16_t*)(input));      \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR565: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB161616_to_BGR565((output), (uint16_t*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB161616_to_RGB565((output), (uint16_t*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB161616_to_BGR888((output), (uint16_t*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB161616_to_BGR8888((output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB161616_to_RGB888((output), (uint16_t*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB161616_to_RGBA8888((output), (uint16_t*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB161616_to_RGB_FLOAT((float**)(output), (uint16_t*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB161616_to_RGBA_FLOAT((float**)(output), (uint16_t*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB161616_to_YUV888((output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB161616_to_YUVA8888((output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV161616: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB161616_to_YUV161616((uint16_t**)(output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA16161616: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB161616_to_YUVA16161616((uint16_t**)(output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV101010: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB161616_to_YUV101010((output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV420P: \
					TRANSFER_YUV420P_OUT_HEAD \
					transfer_RGB161616_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(uint16_t*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					TRANSFER_YUV422P_OUT_HEAD \
					transfer_RGB161616_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(uint16_t*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV444P: \
					TRANSFER_YUV444P_OUT_HEAD \
					transfer_RGB161616_to_YUV444P(output_y, \
						output_u, \
						output_v, \
						(uint16_t*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
		case BC_RGBA16161616: \
			switch(out_colormodel) \
			{ \
				case BC_RGB8: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA16161616_to_RGB8((output), (uint16_t*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR565: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA16161616_to_BGR565((output), (uint16_t*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA16161616_to_RGB565((output), (uint16_t*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888:      \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA16161616_to_BGR888((output), (uint16_t*)(input)); \
					TRANSFER_FRAME_TAIL \
				break; \
				case BC_BGR8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA16161616_to_BGR8888((output), (uint16_t*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA16161616_to_RGB888((output), (uint16_t*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA16161616_to_RGBA8888((output), (uint16_t*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA16161616_to_RGB_FLOAT((float**)(output), (uint16_t*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA_FLOAT: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA16161616_to_RGBA_FLOAT((float**)(output), (uint16_t*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA16161616_to_YUV888((output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA16161616_to_YUVA8888((output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV161616: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA16161616_to_YUV161616(((uint16_t**)output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA16161616: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA16161616_to_YUVA16161616((uint16_t**)(output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV101010: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA16161616_to_YUV101010((output), (uint16_t*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV420P: \
					TRANSFER_YUV420P_OUT_HEAD \
					transfer_RGBA16161616_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(uint16_t*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					TRANSFER_YUV422P_OUT_HEAD \
					transfer_RGBA16161616_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(uint16_t*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV444P: \
					TRANSFER_YUV444P_OUT_HEAD \
					transfer_RGBA16161616_to_YUV444P(output_y, \
						output_u, \
						output_v, \
						(uint16_t*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
		case BC_BGR8888: \
			switch(out_colormodel) \
			{ \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
					transfer_BGR8888_to_RGB888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_FRAME_HEAD \
					transfer_BGR8888_to_BGR8888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
		case BC_BGR888: \
			switch(out_colormodel) \
			{ \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
					transfer_BGR888_to_RGB888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
	} \
}

int64_t get_difference(struct timeval *start_time)
{
        struct timeval new_time;

	gettimeofday(&new_time, 0);

	new_time.tv_usec -= start_time->tv_usec;
	new_time.tv_sec -= start_time->tv_sec;
	if(new_time.tv_usec < 0)
	{
		new_time.tv_usec += 1000000;
		new_time.tv_sec--;
	}

	return (int64_t)new_time.tv_sec * 1000000 + 
		(int64_t)new_time.tv_usec;

}




void cmodel_default(PERMUTATION_ARGS)
{
	if(scale)
	{
		int done = 0;
		switch (in_colormodel) {
			case BC_YUVA8888:
				switch (out_colormodel) {
					case BC_BGR8888:
					{
//						struct timeval start_time;
//						gettimeofday(&start_time, 0);

						short int *utog_tab = yuv_table->utog_tab8;
						short int *vtog_tab = yuv_table->vtog_tab8;
						short int *vtor_tab = yuv_table->vtor_tab8;
						short int *utob_tab = yuv_table->utob_tab8;
						
						short int i ;
						for(i = 0; i < out_h; i++) 
						{ 
					 		unsigned char *input_row = input_rows[row_table[i]]; 
					 		unsigned char *output_row = output_rows[i + out_y] + out_x * out_pixelsize; 
				 			unsigned char *input_data = input_row + (column_table[0] << 2);
							__builtin_prefetch (input_data, 0, 0);

					 		short int j;
					 		for(j = 0; j < out_w; j++) 
					 		{ 	
								unsigned char y = input_data[0];
								unsigned char u = input_data[1];
								unsigned char v = input_data[2];
								unsigned char a = input_data[3];
								
								/* the secret of the speedup ... delayed index calculation */
								/* we do not overflow at the end since column table holds + 1 element */					
								input_data = input_row + (column_table[j + 1] << 2);
								
						 		short int r = y + vtor_tab[v];
								short int g = y + utog_tab[u] + vtog_tab[v];	
								short int b = y + utob_tab[u];
					 			
						 		r = (r < 0 ? 0 : (r > 0xff ? 0xff : r)); 
						 		g = (g < 0 ? 0 : (g > 0xff ? 0xff : g)); 
						 		b = (b < 0 ? 0 : (b > 0xff ? 0xff : b)); 

								r *= a;
								g *= a;
								b *= a;
								
								output_row[0] = ((b) >> 8);
								output_row[1] = ((g) >> 8);
						 	        output_row[2] = ((r) >> 8);
						 	        
						 	        output_row += 4;
					 		} 
				 		
					 	}
//					 	long long dif= get_difference(&start_time);
//						printf("diff_inside: %lli\n", dif);

						done = 1; 
					}
					break;
				}
				break;
				
		}
		if (!done) 
		{
			TRANSFER_FRAME_DEFAULT(&output_row, 
				input_row + column_table[j] * in_pixelsize,
				0,
				0,
				0,
				0);
		}
	}
	else
	{
		TRANSFER_FRAME_DEFAULT(&output_row, 
			input_row + j * in_pixelsize,
			0,
			0,
			0,
			0);
	}
}

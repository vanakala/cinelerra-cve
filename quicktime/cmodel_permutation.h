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
#include "colormodels.h"
#include <stdint.h>


#define RGB_TO_YUV(y, u, v, r, g, b) \
{ \
	y = ((yuv_table->rtoy_tab[r] + yuv_table->gtoy_tab[g] + yuv_table->btoy_tab[b]) >> 16); \
	u = ((yuv_table->rtou_tab[r] + yuv_table->gtou_tab[g] + yuv_table->btou_tab[b]) >> 16); \
	v = ((yuv_table->rtov_tab[r] + yuv_table->gtov_tab[g] + yuv_table->btov_tab[b]) >> 16); \
	RECLIP(y, 0, 0xff); \
	RECLIP(u, 0, 0xff); \
	RECLIP(v, 0, 0xff); \
}

// y -> 24 bits u, v, -> 8 bits r, g, b -> 8 bits
#define YUV_TO_RGB(y, u, v, r, g, b) \
{ \
	(r) = ((y + yuv_table->vtor_tab[v]) >> 16); \
	(g) = ((y + yuv_table->utog_tab[u] + yuv_table->vtog_tab[v]) >> 16); \
	(b) = ((y + yuv_table->utob_tab[u]) >> 16); \
	RECLIP(r, 0, 0xff); \
	RECLIP(g, 0, 0xff); \
	RECLIP(b, 0, 0xff); \
}

// r, g, b -> 16 bits
#define RGB_TO_YUV16(y, u, v, r, g, b) \
{ \
	y = ((yuv_table->rtoy_tab16[r] + yuv_table->gtoy_tab16[g] + yuv_table->btoy_tab16[b]) >> 8); \
	u = ((yuv_table->rtou_tab16[r] + yuv_table->gtou_tab16[g] + yuv_table->btou_tab16[b]) >> 8); \
	v = ((yuv_table->rtov_tab16[r] + yuv_table->gtov_tab16[g] + yuv_table->btov_tab16[b]) >> 8); \
	RECLIP(y, 0, 0xffff); \
	RECLIP(u, 0, 0xffff); \
	RECLIP(v, 0, 0xffff); \
}

// y -> 24 bits   u, v-> 16 bits
#define YUV_TO_RGB16(y, u, v, r, g, b) \
{ \
	(r) = ((y + yuv_table->vtor_tab16[v]) >> 8); \
	(g) = ((y + yuv_table->utog_tab16[u] + yuv_table->vtog_tab16[v]) >> 8); \
	(b) = ((y + yuv_table->utob_tab16[u]) >> 8); \
	RECLIP(r, 0, 0xffff); \
	RECLIP(g, 0, 0xffff); \
	RECLIP(b, 0, 0xffff); \
}


#define WRITE_YUV101010(y, u, v) \
{ \
	uint32_t output_i = ((y & 0xffc0) << 16) | \
		((u & 0xffc0) << 6) | \
		((v & 0xffc0) >> 4); \
	*(*output)++ = (output_i & 0xff); \
	*(*output)++ = (output_i & 0xff00) >> 8; \
	*(*output)++ = (output_i & 0xff0000) >> 16; \
	*(*output)++ = (output_i & 0xff000000) >> 24; \
}



// ****************************** Pixel transfers *****************************






// ****************************** ARGB8888 -> *********************************

static inline void transfer_ARGB8888_to_ARGB8888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[0];
	(*output)[1] = input[1];
	(*output)[2] = input[2];
	(*output)[3] = input[3];
	(*output) += 4;
}















// ******************************** RGB888 -> *********************************

static inline void transfer_RGB888_to_RGB8(unsigned char *(*output), unsigned char *input)
{
	*(*output) = (unsigned char)((input[0] & 0xc0) +
			    			 ((input[1] & 0xe0) >> 2) +
		 	    			 ((input[2] & 0xe0) >> 5));
	(*output)++;
}

static inline void transfer_RGB888_to_BGR565(unsigned char *(*output), unsigned char *input)
{
	uint16_t r, g, b;
	uint16_t r_s, g_s, b_s;
	r = *input++;
	g = *input++;
	b = *input;
	
	r_s  = (r & 0x01) << 7;
	r_s |= (r & 0x02) << 5;
	r_s |= (r & 0x04) << 3;
	r_s |= (r & 0x08) << 1;
	r_s |= (r & 0x10) >> 1;
	r_s |= (r & 0x20) >> 3;
	r_s |= (r & 0x40) >> 5;
	r_s |= (r & 0x80) >> 7;

	g_s  = (g & 0x01) << 7;
	g_s |= (g & 0x02) << 5;
	g_s |= (g & 0x04) << 3;
	g_s |= (g & 0x08) << 1;
	g_s |= (g & 0x10) >> 1;
	g_s |= (g & 0x20) >> 3;
	g_s |= (g & 0x40) >> 5;
	g_s |= (g & 0x80) >> 7;

	b_s  = (b & 0x01) << 7;
	b_s |= (b & 0x02) << 5;
	b_s |= (b & 0x04) << 3;
	b_s |= (b & 0x08) << 1;
	b_s |= (b & 0x10) >> 1;
	b_s |= (b & 0x20) >> 3;
	b_s |= (b & 0x40) >> 5;
	b_s |= (b & 0x80) >> 7;

	*(uint16_t*)(*output) = ((b_s & 0xf8) << 8)
			 + ((g_s & 0xfc) << 3)
			 + ((r_s & 0xf8) >> 3);
	(*output) += 2;
}

static inline void transfer_RGB888_to_RGB565(unsigned char *(*output), unsigned char *input)
{
	uint16_t r, g, b;
	r = *input++;
	g = *input++;
	b = *input;
	*(uint16_t*)(*output) = ((r & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((b & 0xf8) >> 3);
	(*output) += 2;
}

static inline void transfer_RGB888_to_BGR888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[2];
	(*output)[1] = input[1];
	(*output)[2] = input[0];
	(*output) += 3;
}

static inline void transfer_RGB888_to_RGB888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[0];
	(*output)[1] = input[1];
	(*output)[2] = input[2];
	(*output) += 3;
}

static inline void transfer_RGB888_to_RGBA8888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[0];
	(*output)[1] = input[1];
	(*output)[2] = input[2];
	(*output)[3] = 0xff;
	(*output) += 4;
}

static inline void transfer_RGB888_to_RGB161616(uint16_t *(*output), unsigned char *input)
{
	(*output)[0] = (input[0] << 8) | input[0];
	(*output)[1] = (input[1] << 8) | input[1];
	(*output)[2] = (input[2] << 8) | input[2];
	(*output) += 3;
}

static inline void transfer_RGB888_to_RGBA16161616(uint16_t *(*output), unsigned char *input)
{
	(*output)[0] = (input[0] << 8) | input[0];
	(*output)[1] = (input[1] << 8) | input[1];
	(*output)[2] = (input[2] << 8) | input[2];
	(*output)[3] = 0xffff;
	(*output) += 4;
}

static inline void transfer_RGB888_to_ARGB8888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = 0xff;
	(*output)[1] = input[0];
	(*output)[2] = input[1];
	(*output)[3] = input[2];
	(*output) += 4;
}

static inline void transfer_RGB888_to_ABGR8888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = 0xff;
	(*output)[1] = input[2];
	(*output)[2] = input[1];
	(*output)[3] = input[0];
	(*output) += 4;
}

static inline void transfer_RGB888_to_BGR8888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[2];
	(*output)[1] = input[1];
	(*output)[2] = input[0];
	(*output) += 4;
}

static inline void transfer_RGB888_to_YUV888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	(*output)[0] = y;
	(*output)[1] = u;
	(*output)[2] = v;
	(*output) += 3;
}


static inline void transfer_RGB888_to_YUV101010(unsigned char *(*output), unsigned char *input)
{
	int r, g, b;
	int y, u, v;

	r = ((uint16_t)input[0]) << 8;
	g = ((uint16_t)input[1]) << 8;
	b = ((uint16_t)input[2]) << 8;
	RGB_TO_YUV16(y, u, v, r, g, b);
	WRITE_YUV101010(y, u, v);
}

static inline void transfer_RGB888_to_VYU888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	(*output)[0] = v;
	(*output)[1] = y;
	(*output)[2] = u;
	(*output) += 3;
}

static inline void transfer_RGB888_to_UYVA8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	(*output)[0] = u;
	(*output)[1] = y;
	(*output)[2] = v;
	(*output)[3] = 0xff;
	(*output) += 4;
}



static inline void transfer_RGB888_to_YUVA8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	(*output)[0] = y;
	(*output)[1] = u;
	(*output)[2] = v;
	(*output)[3] = 255;
	(*output) += 4;
}

static inline void transfer_RGB888_to_YUV161616(uint16_t *(*output), unsigned char *input)
{
	int y, u, v, opacity, r, g, b;
	
	r = ((int)input[0] << 8) | input[0];
	g = ((int)input[1] << 8) | input[1];
	b = ((int)input[2] << 8) | input[2];

	RGB_TO_YUV16(y, u, v, r, g, b);

	(*output)[0] = y;
	(*output)[1] = u;
	(*output)[2] = v;
	(*output) += 3;
}

static inline void transfer_RGB888_to_YUVA16161616(uint16_t *(*output), unsigned char *input)
{
	int y, u, v, r, g, b;

	r = (((int)input[0]) << 8) | input[0];
	g = (((int)input[1]) << 8) | input[1];
	b = (((int)input[2]) << 8) | input[2];
	RGB_TO_YUV16(y, u, v, r, g, b);

	(*output)[0] = y;
	(*output)[1] = u;
	(*output)[2] = v;
	(*output)[3] = 0xffff;
	(*output) += 4;
}

static inline void transfer_RGB888_to_YUV420P_YUV422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column)
{
	int y, u, v;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	output_y[output_column] = y;
	output_u[output_column / 2] = u;
	output_v[output_column / 2] = v;
}

static inline void transfer_RGB888_to_YUV444P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column)
{
	int y, u, v;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	output_y[output_column] = y;
	output_u[output_column] = u;
	output_v[output_column] = v;
}

static inline void transfer_RGB888_to_YUV422(unsigned char *(*output), 
	unsigned char *input,
	int j)
{
	int y, u, v;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	if(!(j & 1))
	{ 
// Store U and V for even pixels only
		 (*output)[1] = u;
		 (*output)[3] = v;
		 (*output)[0] = y;
	}
	else
	{ 
// Store Y and advance output for odd pixels only
		 (*output)[2] = y;
		 (*output) += 4;
	}

}







// *************************** RGBA8888 -> ************************************

static inline void transfer_RGBA8888_to_TRANSPARENCY(unsigned char *(*output), unsigned char *input, int (*bit_counter))
{
	if((*bit_counter) == 7) *(*output) = 0;

	if(input[3] < 127) 
	{
		*(*output) |= (unsigned char)1 << (7 - (*bit_counter));
	}

	if((*bit_counter) == 0)
	{
		(*output)++;
		(*bit_counter) = 7;
	}
	else
		(*bit_counter)--;
}

// These routines blend in a background color since they should be
// exclusively used for widgets.

static inline void transfer_RGBA8888_to_RGB8bg(unsigned char *(*output), unsigned char *input, int bg_r, int bg_g, int bg_b)
{
	unsigned int r, g, b, a, anti_a;
	a = input[3];
	anti_a = 255 - a;
	r = ((unsigned int)input[0] * a + bg_r * anti_a) / 0xff;
	g = ((unsigned int)input[1] * a + bg_g * anti_a) / 0xff;
	b = ((unsigned int)input[2] * a + bg_b * anti_a) / 0xff;
	*(*output) = (unsigned char)((r & 0xc0) + 
				((g & 0xe0) >> 2) + 
				((b & 0xe0) >> 5));
	(*output)++;
}

static inline void transfer_RGBA8888_to_BGR565bg(unsigned char *(*output), unsigned char *input, int bg_r, int bg_g, int bg_b)
{
	unsigned int r, g, b, a, anti_a;
	a = input[3];
	anti_a = 255 - a;
	r = ((unsigned int)input[0] * a + bg_r * anti_a) / 0xff;
	g = ((unsigned int)input[1] * a + bg_g * anti_a) / 0xff;
	b = ((unsigned int)input[2] * a + bg_b * anti_a) / 0xff;
	*(uint16_t*)(*output) = (uint16_t)(((b & 0xf8) << 8) + 
				((g & 0xfc) << 3) + 
				((r & 0xf8) >> 3));
	(*output) += 2;
}

static inline void transfer_RGBA8888_to_RGB565bg(unsigned char *(*output), unsigned char *input, int bg_r, int bg_g, int bg_b)
{
	unsigned int r, g, b, a, anti_a;
	a = input[3];
	anti_a = 255 - a;
	r = ((unsigned int)input[0] * a + bg_r * anti_a) / 0xff;
	g = ((unsigned int)input[1] * a + bg_g * anti_a) / 0xff;
	b = ((unsigned int)input[2] * a + bg_b * anti_a) / 0xff;
	*(uint16_t*)(*output) = (uint16_t)(((r & 0xf8) << 8)+ 
				((g & 0xfc) << 3) + 
				((b & 0xf8) >> 3));
	(*output) += 2;
}

static inline void transfer_RGBA8888_to_BGR888bg(unsigned char *(*output), unsigned char *input, int bg_r, int bg_g, int bg_b)
{
	unsigned int r, g, b, a, anti_a;
	a = input[3];
	anti_a = 255 - a;
	r = ((unsigned int)input[0] * a + bg_r * anti_a) / 0xff;
	g = ((unsigned int)input[1] * a + bg_g * anti_a) / 0xff;
	b = ((unsigned int)input[2] * a + bg_b * anti_a) / 0xff;
	(*output)[0] = b;
	(*output)[1] = g;
	(*output)[2] = r;
	(*output) += 3;
}

static inline void transfer_RGBA8888_to_RGB888bg(unsigned char *(*output), unsigned char *input, int bg_r, int bg_g, int bg_b)
{
	unsigned int r, g, b, a, anti_a;
	a = input[3];
	anti_a = 255 - a;
	r = ((unsigned int)input[0] * a + bg_r * anti_a) / 0xff;
	g = ((unsigned int)input[1] * a + bg_g * anti_a) / 0xff;
	b = ((unsigned int)input[2] * a + bg_b * anti_a) / 0xff;
	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output) += 3;
}

static inline void transfer_RGBA8888_to_BGR8888bg(unsigned char *(*output), unsigned char *input, int bg_r, int bg_g, int bg_b)
{
	unsigned int r, g, b, a, anti_a;
	a = input[3];
	anti_a = 255 - a;

	r = ((unsigned int)input[0] * a + bg_r * anti_a) / 0xff;
	g = ((unsigned int)input[1] * a + bg_g * anti_a) / 0xff;
	b = ((unsigned int)input[2] * a + bg_b * anti_a) / 0xff;

	(*output)[0] = b;
	(*output)[1] = g;
	(*output)[2] = r;
	(*output) += 4;
}







// These routines blend in a black background

static inline void transfer_RGBA8888_to_RGB8(unsigned char *(*output), unsigned char *input)
{
	unsigned int r, g, b, a;
	a = input[3];
	r = (unsigned int)input[0] * a;
	g = (unsigned int)input[1] * a;
	b = (unsigned int)input[2] * a;
	*(*output) = (unsigned char)(((r & 0xc000) >> 8) + 
				((g & 0xe000) >> 10) + 
				((b & 0xe000) >> 13));
	(*output)++;
}

static inline void transfer_RGBA8888_to_BGR565(unsigned char *(*output), unsigned char *input)
{
	unsigned int r, g, b, a;
	a = input[3];
	r = ((unsigned int)input[0] * a) / 0xff;
	g = ((unsigned int)input[1] * a) / 0xff;
	b = ((unsigned int)input[2] * a) / 0xff;
	*(uint16_t*)(*output) = (uint16_t)(((b & 0xf8) << 8) + 
				((g & 0xfc) << 3) + 
				((r & 0xf8) >> 3));
	(*output) += 2;
}

static inline void transfer_RGBA8888_to_RGB565(unsigned char *(*output), unsigned char *input)
{
	unsigned int r, g, b, a;
	a = input[3];
	r = ((unsigned int)input[0] * a) / 0xff;
	g = ((unsigned int)input[1] * a) / 0xff;
	b = ((unsigned int)input[2] * a) / 0xff;


	*(uint16_t*)(*output) = (uint16_t)(((r & 0xf8) << 8) + 
				((g & 0xfc) << 3) + 
				((b & 0xf8) >> 3));
	(*output) += 2;
}

static inline void transfer_RGBA8888_to_BGR888(unsigned char *(*output), unsigned char *input)
{
	unsigned int r, g, b, a;
	a = input[3];
	r = ((unsigned int)input[0] * a) / 0xff;
	g = ((unsigned int)input[1] * a) / 0xff;
	b = ((unsigned int)input[2] * a) / 0xff;
	(*output)[0] = b;
	(*output)[1] = g;
	(*output)[2] = r;
	(*output) += 3;
}

static inline void transfer_RGBA8888_to_RGB888(unsigned char *(*output), unsigned char *input)
{
	unsigned int r, g, b, a;
	a = input[3];
	r = ((unsigned int)input[0] * a) / 0xff;
	g = ((unsigned int)input[1] * a) / 0xff;
	b = ((unsigned int)input[2] * a) / 0xff;
	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output) += 3;
}

static inline void transfer_RGBA8888_to_RGBA8888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[0];
	(*output)[1] = input[1];
	(*output)[2] = input[2];
	(*output)[3] = input[3];
	(*output) += 4;
}

static inline void transfer_RGBA8888_to_RGB161616(uint16_t *(*output), unsigned char *input)
{
	int y, u, v, opacity, r, g, b;
	
	opacity = input[3];
	(*output)[0] = (((int)input[0] << 8) | input[0]) * opacity / 0xff;
	(*output)[1] = (((int)input[1] << 8) | input[1]) * opacity / 0xff;
	(*output)[2] = (((int)input[2] << 8) | input[2]) * opacity / 0xff;
	(*output) += 3;
}

static inline void transfer_RGBA8888_to_RGBA16161616(uint16_t *(*output), unsigned char *input)
{
	int y, u, v, r, g, b;

	(*output)[0] = (((int)input[0]) << 8) | input[0];
	(*output)[1] = (((int)input[1]) << 8) | input[1];
	(*output)[2] = (((int)input[2]) << 8) | input[2];
	(*output)[3] = (((int)input[3]) << 8) | input[3];
	(*output) += 4;
}

static inline void transfer_RGBA8888_to_BGR8888(unsigned char *(*output), unsigned char *input)
{
	unsigned int r, g, b, a;
	a = input[3];
	r = ((unsigned int)input[0] * a) / 0xff;
	g = ((unsigned int)input[1] * a) / 0xff;
	b = ((unsigned int)input[2] * a) / 0xff;
	(*output)[0] = b;
	(*output)[1] = g;
	(*output)[2] = r;
	(*output) += 4;
}

static inline void transfer_RGBA8888_to_YUV888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v, a, r, g, b;
	
	a = input[3];
	r = (input[0] * a) / 0xff;
	g = (input[1] * a) / 0xff;
	b = (input[2] * a) / 0xff;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	(*output)[0] = y;
	(*output)[1] = u;
	(*output)[2] = v;
	(*output) += 3;
}

static inline void transfer_RGBA8888_to_YUVA8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	(*output)[0] = y;
	(*output)[1] = u;
	(*output)[2] = v;
	(*output)[3] = input[3];
	(*output) += 4;
}

static inline void transfer_RGBA8888_to_YUV161616(uint16_t *(*output), unsigned char *input)
{
	int y, u, v, opacity, r, g, b;
	
	opacity = input[3];
	r = (((int)input[0] << 8) | input[0]) * opacity / 0xff;
	g = (((int)input[1] << 8) | input[1]) * opacity / 0xff;
	b = (((int)input[2] << 8) | input[2]) * opacity / 0xff;

	RGB_TO_YUV16(y, u, v, r, g, b);

	(*output)[0] = y;
	(*output)[1] = u;
	(*output)[2] = v;
	(*output) += 3;
}

static inline void transfer_RGBA8888_to_YUVA16161616(uint16_t *(*output), unsigned char *input)
{
	int y, u, v, r, g, b;

	r = (((int)input[0]) << 8) | input[0];
	g = (((int)input[1]) << 8) | input[1];
	b = (((int)input[2]) << 8) | input[2];
	RGB_TO_YUV16(y, u, v, r, g, b);

	(*output)[0] = y;
	(*output)[1] = u;
	(*output)[2] = v;
	(*output)[3] = (((int)input[3]) << 8) | input[3];
	(*output) += 4;
}

static inline void transfer_RGBA8888_to_YUV101010(unsigned char *(*output), unsigned char *input)
{
	int r, g, b;
	int y, u, v;

	r = ((uint16_t)input[0] * input[3]) + 0x1fe;
	g = ((uint16_t)input[1] * input[3]) + 0x1fe;
	b = ((uint16_t)input[2] * input[3]) + 0x1fe;
	RGB_TO_YUV16(y, u, v, r, g, b);
	WRITE_YUV101010(y, u, v);
}

static inline void transfer_RGBA8888_to_VYU888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v, a, r, g, b;
	
	a = input[3];
	r = ((input[0] * a) >> 8) + 1;
	g = ((input[1] * a) >> 8) + 1;
	b = ((input[2] * a) >> 8) + 1;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	(*output)[0] = v;
	(*output)[1] = y;
	(*output)[2] = u;
	(*output) += 3;
}

static inline void transfer_RGBA8888_to_UYVA8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	(*output)[0] = u;
	(*output)[1] = y;
	(*output)[2] = v;
	(*output)[3] = input[3];
	(*output) += 4;
}

static inline void transfer_RGBA888_to_YUV420P_YUV422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column)
{
	int y, u, v, a, r, g, b;
	
	a = input[3];
	r = (input[0] * a) / 0xff;
	g = (input[1] * a) / 0xff;
	b = (input[2] * a) / 0xff;

	RGB_TO_YUV(y, u, v, r, g, b);

	output_y[output_column] = y;
	output_u[output_column / 2] = u;
	output_v[output_column / 2] = v;
}

static inline void transfer_RGBA888_to_YUV444P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column)
{
	int y, u, v, a, r, g, b;
	
	a = input[3];
	r = (input[0] * a) / 0xff;
	g = (input[1] * a) / 0xff;
	b = (input[2] * a) / 0xff;

	RGB_TO_YUV(y, u, v, r, g, b);

	output_y[output_column] = y;
	output_u[output_column] = u;
	output_v[output_column] = v;
}

static inline void transfer_RGBA888_to_YUV422(unsigned char *(*output), 
	unsigned char *input,
	int j)
{
	int y, u, v, a, r, g, b;
	
	a = input[3];
	r = (input[0] * a) / 0xff;
	g = (input[1] * a) / 0xff;
	b = (input[2] * a) / 0xff;

	RGB_TO_YUV(y, u, v, r, g, b);

	if(!(j & 1))
	{ 
// Store U and V for even pixels only
		 (*output)[1] = u;
		 (*output)[3] = v;
		 (*output)[0] = y;
	}
	else
	{ 
// Store Y and advance output for odd pixels only
		 (*output)[2] = y;
		 (*output) += 4;
	}

}













// ******************************** RGB161616 -> *********************************

static inline void transfer_RGB161616_to_RGB8(unsigned char *(*output), uint16_t *input)
{
	*(*output) = (unsigned char)(((input[0] & 0xc000) >> 8) +
			    			 ((input[1] & 0xe000) >> 10) +
		 	    			 ((input[2] & 0xe000) >> 13));
	(*output)++;
}

static inline void transfer_RGB161616_to_BGR565(unsigned char *(*output), uint16_t *input)
{
	uint16_t r, g, b;
	r = *input++;
	g = *input++;
	b = *input;
	*(uint16_t*)(*output) = (b & 0xf800) |
			 ((g & 0xfc00) >> 5) |
			 ((r & 0xf800) >> 11);
	(*output) += 2;
}

static inline void transfer_RGB161616_to_RGB565(unsigned char *(*output), uint16_t *input)
{
	uint16_t r, g, b;
	r = *input++;
	g = *input++;
	b = *input;
	*(uint16_t*)(*output) = (r & 0xf800) |
			 ((g & 0xfc00) >> 5) |
			 ((b & 0xf800) >> 11);
	(*output) += 2;
}

static inline void transfer_RGB161616_to_BGR888(unsigned char *(*output), uint16_t *input)
{
	(*output)[0] = input[2] >> 8;
	(*output)[1] = input[1] >> 8;
	(*output)[2] = input[0] >> 8;
	(*output) += 3;
}

static inline void transfer_RGB161616_to_RGB888(unsigned char *(*output), uint16_t *input)
{
	(*output)[0] = input[0] >> 8;
	(*output)[1] = input[1] >> 8;
	(*output)[2] = input[2] >> 8;
	(*output) += 3;
}

static inline void transfer_RGB161616_to_RGBA8888(unsigned char *(*output), uint16_t *input)
{
	(*output)[0] = input[0] >> 8;
	(*output)[1] = input[1] >> 8;
	(*output)[2] = input[2] >> 8;
	(*output)[3] = 0xff;
	(*output) += 4;
}

static inline void transfer_RGB161616_to_BGR8888(unsigned char *(*output), uint16_t *input)
{
	(*output)[0] = input[2] >> 8;
	(*output)[1] = input[1] >> 8;
	(*output)[2] = input[0] >> 8;
	(*output) += 4;
}

static inline void transfer_RGB161616_to_YUV888(unsigned char *(*output), uint16_t *input)
{
	int y, u, v, r, g, b;
	r = input[0] >> 8;
	g = input[1] >> 8;
	b = input[2] >> 8;

	RGB_TO_YUV(y, u, v, r, g, b);

	(*output)[0] = y;
	(*output)[1] = u;
	(*output)[2] = v;
	(*output) += 3;
}

static inline void transfer_RGB161616_to_YUVA8888(unsigned char *(*output), uint16_t *input)
{
	int y, u, v, r, g, b;

	r = input[0] >> 8;
	g = input[1] >> 8;
	b = input[2] >> 8;

	RGB_TO_YUV(y, u, v, r, g, b);

	(*output)[0] = y;
	(*output)[1] = u;
	(*output)[2] = v;
	(*output)[3] = 255;
	(*output) += 4;
}


static inline void transfer_RGB161616_to_YUV101010(unsigned char *(*output), uint16_t *input)
{
	int r, g, b;
	int y, u, v;

	r = input[0];
	g = input[1];
	b = input[2];
	RGB_TO_YUV16(y, u, v, r, g, b);
	WRITE_YUV101010(y, u, v);
}

static inline void transfer_RGB161616_to_VYU888(unsigned char *(*output), uint16_t *input)
{
	int y, u, v, r, g, b;
	r = input[0] >> 8;
	g = input[1] >> 8;
	b = input[2] >> 8;

	RGB_TO_YUV(y, u, v, r, g, b);

	(*output)[0] = v;
	(*output)[1] = y;
	(*output)[2] = u;
	(*output) += 3;
}

static inline void transfer_RGB161616_to_UYVA8888(unsigned char *(*output), uint16_t *input)
{
	int y, u, v, r, g, b;

	r = input[0] >> 8;
	g = input[1] >> 8;
	b = input[2] >> 8;

	RGB_TO_YUV(y, u, v, r, g, b);

	(*output)[0] = u;
	(*output)[1] = y;
	(*output)[2] = v;
	(*output)[3] = 0xff;
	(*output) += 4;
}


static inline void transfer_RGB161616_to_YUV420P_YUV422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	uint16_t *input,
	int output_column)
{
	int y, u, v, r, g, b;
	r = input[0] >> 8;
	g = input[1] >> 8;
	b = input[2] >> 8;

	RGB_TO_YUV(y, u, v, r, g, b);

	output_y[output_column] = y;
	output_u[output_column / 2] = u;
	output_v[output_column / 2] = v;
}

static inline void transfer_RGB161616_to_YUV444P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	uint16_t *input,
	int output_column)
{
	int y, u, v, r, g, b;
	r = input[0] >> 8;
	g = input[1] >> 8;
	b = input[2] >> 8;

	RGB_TO_YUV(y, u, v, r, g, b);

	output_y[output_column] = y;
	output_u[output_column] = u;
	output_v[output_column] = v;
}


// ****************************** RGBA16161616 -> *****************************

static inline void transfer_RGBA16161616_to_RGB8(unsigned char *(*output), uint16_t *input)
{
	unsigned int r, g, b, a;
	a = (input)[3] >> 8;
	r = (unsigned int)(input)[0] * a;
	g = (unsigned int)(input)[1] * a;
	b = (unsigned int)(input)[2] * a;

	*(*output) = (unsigned char)(((r & 0xc00000) >> 16) + 
				((g & 0xe00000) >> 18) + 
				((b & 0xe00000) >> 21));
	(*output)++;
}

static inline void transfer_RGBA16161616_to_BGR565(unsigned char *(*output), uint16_t *input)
{
	unsigned int r, g, b, a;
	a = (input)[3] >> 8;
	r = (unsigned int)(input)[0] * a;
	g = (unsigned int)(input)[1] * a;
	b = (unsigned int)(input)[2] * a;

	*(uint16_t*)(*output) = (uint16_t)(((b & 0xf80000) >> 8) + 
				((g & 0xfc0000) >> 13) + 
				((r & 0xf80000) >> 19));
	(*output) += 2;
}

static inline void transfer_RGBA16161616_to_RGB565(unsigned char *(*output), uint16_t *input)
{
	unsigned int r, g, b, a;
	a = (input)[3] >> 8;
	r = (unsigned int)(input)[0] * a;
	g = (unsigned int)(input)[1] * a;
	b = (unsigned int)(input)[2] * a;

	*(uint16_t*)(*output) = (uint16_t)(((r & 0xf80000) >> 8) + 
				((g & 0xfc0000) >> 13) + 
				((b & 0xf80000) >> 19));
	(*output) += 2;
}

static inline void transfer_RGBA16161616_to_BGR888(unsigned char *(*output), uint16_t *input)
{
	unsigned int r, g, b, a;
	a = (input)[3] >> 8;
	r = (unsigned int)(input)[0] * a;
	g = (unsigned int)(input)[1] * a;
	b = (unsigned int)(input)[2] * a;

	(*output)[0] = (unsigned char)(b >> 16);
	(*output)[1] = (unsigned char)(g >> 16);
	(*output)[2] = (unsigned char)(r >> 16);
	(*output) += 3;
}

static inline void transfer_RGBA16161616_to_RGB888(unsigned char *(*output), uint16_t *input)
{
	unsigned int r, g, b, a;
	a = (input)[3] >> 8;
	r = (unsigned int)(input)[0] * a;
	g = (unsigned int)(input)[1] * a;
	b = (unsigned int)(input)[2] * a;

	(*output)[0] = (unsigned char)(r >> 16);
	(*output)[1] = (unsigned char)(g >> 16);
	(*output)[2] = (unsigned char)(b >> 16);
	(*output) += 3;
}


static inline void transfer_RGBA16161616_to_RGBA8888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[0] >> 8;
	(*output)[1] = input[1] >> 8;
	(*output)[2] = input[2] >> 8;
	(*output)[3] = input[3] >> 8;
	(*output) += 4;
}


static inline void transfer_RGBA16161616_to_BGR8888(unsigned char *(*output), uint16_t *input)
{
	unsigned int r, g, b, a;
	a = (input)[3] >> 8;
	r = (input)[0] * a;
	g = (input)[1] * a;
	b = (input)[2] * a;

	(*output)[0] = (unsigned char)(b >> 16);
	(*output)[1] = (unsigned char)(g >> 16);
	(*output)[2] = (unsigned char)(r >> 16);
	(*output) += 4;
}

static inline void transfer_RGBA16161616_to_YUV101010(unsigned char *(*output), uint16_t *input)
{
	int r, g, b;
	int y, u, v;

	r = (((uint32_t)input[0] * input[3]) >> 16) + 0x1;
	g = (((uint32_t)input[1] * input[3]) >> 16) + 0x1;
	b = (((uint32_t)input[2] * input[3]) >> 16) + 0x1;
	RGB_TO_YUV16(y, u, v, r, g, b);
	WRITE_YUV101010(y, u, v);
}


static inline void transfer_RGBA16161616_to_YUV420P_YUV422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	uint16_t *input,
	int output_column)
{
	int y, u, v, r, g, b;
	int64_t a;
	a = input[3];
	r = (int64_t)input[0] * a / 0xffffff;
	g = (int64_t)input[1] * a / 0xffffff;
	b = (int64_t)input[2] * a / 0xffffff;

	RGB_TO_YUV(y, u, v, r, g, b);

	output_y[output_column] = y;
	output_u[output_column / 2] = u;
	output_v[output_column / 2] = v;
}

static inline void transfer_RGBA16161616_to_YUV444P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	uint16_t *input,
	int output_column)
{
	int y, u, v, r, g, b;
	int64_t a;
	a = input[3];
	r = (int64_t)input[0] * a / 0xffffff;
	g = (int64_t)input[1] * a / 0xffffff;
	b = (int64_t)input[2] * a / 0xffffff;

	RGB_TO_YUV(y, u, v, r, g, b);

	output_y[output_column] = y;
	output_u[output_column] = u;
	output_v[output_column] = v;
}

















static inline void transfer_BGR8888_to_RGB888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[2];
	(*output)[1] = input[1];
	(*output)[2] = input[0];
	(*output) += 3;
}

static inline void transfer_BGR8888_to_BGR8888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[0];
	(*output)[1] = input[1];
	(*output)[2] = input[2];
	(*output) += 4;
}

static inline void transfer_BGR888_to_RGB888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[2];
	(*output)[1] = input[1];
	(*output)[2] = input[0];
	(*output) += 3;
}







// ******************************** YUV888 -> *********************************


static inline void transfer_YUV888_to_RGB8(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = (input[0] << 16);
	u = input[1];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);

	*(*output) = (unsigned char)((r & 0xc0) +
			    			 ((g & 0xe0) >> 2) +
		 	    			 ((b & 0xe0) >> 5));
	(*output)++;
}

static inline void transfer_YUV888_to_BGR565(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[0]) << 16;
	u = input[1];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
	*(uint16_t*)(*output) = ((b & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((r & 0xf8) >> 3);
	(*output) += 2;
}

static inline void transfer_YUV888_to_RGB565(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[0]) << 16;
	u = input[1];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
	*(uint16_t*)(*output) = ((r & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((b & 0xf8) >> 3);
	(*output) += 2;
}

static inline void transfer_YUV888_to_BGR888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[0]) << 16;
	u = input[1];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);

	(*output)[2] = r;
	(*output)[1] = g;
	(*output)[0] = b;
	(*output) += 3;
}

static inline void transfer_YUV888_to_BGR8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;

	y = ((int)input[0]) << 16;
	u = input[1];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
	(*output)[2] = r;
	(*output)[1] = g;
	(*output)[0] = b;
	(*output) += 4;
}

static inline void transfer_YUV888_to_RGB888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[0]) << 16;
	u = input[1];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output) += 3;
}

static inline void transfer_YUV888_to_RGBA8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;

	y = ((int)input[0]) << 16;
	u = input[1];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output)[3] = 0xff;
	(*output) += 4;
}

static inline void transfer_YUV888_to_YUVA8888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = (int)input[0];
	(*output)[1] = input[1];
	(*output)[2] = input[2];
	(*output)[3] = 0xff;
	(*output) += 4;
}

static inline void transfer_YUV888_to_YUV888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = (int)input[0];
	(*output)[1] = input[1];
	(*output)[2] = input[2];
	(*output) += 3;
}


static inline void transfer_YUV888_to_VYU888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[2];
	(*output)[1] = input[0];
	(*output)[2] = input[1];
	(*output) += 3;
}


static inline void transfer_YUV888_to_UYVA8888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[1];
	(*output)[1] = input[0];
	(*output)[2] = input[2];
	(*output)[3] = 0xff;
	(*output) += 4;
}


static inline void transfer_YUV888_to_YUV101010(unsigned char *(*output), unsigned char *input)
{
	uint16_t y_i = ((uint16_t)input[0]) << 8;
	uint16_t u_i = ((uint16_t)input[1]) << 8;
	uint16_t v_i = ((uint16_t)input[2]) << 8;
	WRITE_YUV101010(y_i, u_i, v_i);
}

static inline void transfer_YUV888_to_YUV420P_YUV422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column)
{
	output_y[output_column] = input[0];
	output_u[output_column / 2] = input[1];
	output_v[output_column / 2] = input[2];
}

static inline void transfer_YUV888_to_YUV444P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column)
{
	output_y[output_column] = input[0];
	output_u[output_column] = input[1];
	output_v[output_column] = input[2];
}

static inline void transfer_YUV888_to_YUV422(unsigned char *(*output), 
	unsigned char *input,
	int j)
{
// Store U and V for even pixels only
	if(!(j & 1))
	{
		(*output)[1] = input[1];
		(*output)[3] = input[2];
		(*output)[0] = input[0];
	}
	else
// Store Y and advance output for odd pixels only
	{
		(*output)[2] = input[0];
		(*output) += 4;
	}
}






// ******************************** YUVA8888 -> *******************************




static inline void transfer_YUVA8888_to_RGB8(unsigned char *(*output), unsigned char *input)
{
	int y, u, v, a;
	int r, g, b;
	
	a = input[3];
	y = ((int)input[0]) << 16;
	u = input[1];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
	
	r *= a;
	g *= a;
	b *= a;

	*(*output) = (unsigned char)(((r & 0xc000) >> 8) + 
				((g & 0xe000) >> 10) + 
				((b & 0xe000) >> 13));
	(*output)++;
}

static inline void transfer_YUVA8888_to_BGR565(unsigned char *(*output), unsigned char *input)
{
	int y, u, v, a;
	int r, g, b;
	
	a = input[3];
	y = ((int)input[0]) << 16;
	u = input[1];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
		
	r *= a;
	g *= a;
	b *= a;

	*(uint16_t*)(*output) = (uint16_t)((b & 0xf800) + 
				((g & 0xfc00) >> 5) + 
				((r & 0xf800) >> 11));
	(*output) += 2;
}

static inline void transfer_YUVA8888_to_RGB565(unsigned char *(*output), unsigned char *input)
{
	int y, u, v, a;
	int r, g, b;
	
	a = input[3];
	y = ((int)input[0]) << 16;
	u = input[1];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
		
	r *= a;
	g *= a;
	b *= a;

	*(uint16_t*)(*output) = (uint16_t)((r & 0xf800) + 
				((g & 0xfc00) >> 5) + 
				((b & 0xf800) >> 11));
	(*output) += 2;
}

static inline void transfer_YUVA8888_to_BGR888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v, a;
	int r, g, b;
	
	a = input[3];
	y = ((int)input[0]) << 16;
	u = input[1];
	v = input[2];

	YUV_TO_RGB(y, u, v, r, g, b);
		
	r *= a;
	g *= a;
	b *= a;

	(*output)[0] = (b >> 8) + 1;
	(*output)[1] = (g >> 8) + 1;
	(*output)[2] = (r >> 8) + 1;
	(*output) += 3;
}

static inline void transfer_YUVA8888_to_RGB888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v, a;
	int r, g, b;
	
	a = input[3];
	y = ((int)input[0]) << 16;
	u = input[1];
	v = input[2];

	YUV_TO_RGB(y, u, v, r, g, b);
		
	r *= a;
	g *= a;
	b *= a;

	(*output)[0] = (r >> 8) + 1;
	(*output)[1] = (g >> 8) + 1;
	(*output)[2] = (b >> 8) + 1;
	(*output) += 3;
}

static inline void transfer_YUVA8888_to_RGBA8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v, a;
	int r, g, b;

	a = input[3];
	y = ((int)input[0]) << 16;
	u = input[1];
	v = input[2];

	YUV_TO_RGB(y, u, v, r, g, b);
	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output)[3] = a;
	(*output) += 4;
}

static inline void transfer_YUVA8888_to_BGR8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v, a;
	int r, g, b;

	a = input[3];
	y = ((int)input[0]) << 16;
	u = input[1];
	v = input[2];

	YUV_TO_RGB(y, u, v, r, g, b);
	
	r *= a;
	g *= a;
	b *= a;
	(*output)[0] = (b >> 8) + 1;
	(*output)[1] = (g >> 8) + 1;
	(*output)[2] = (r >> 8) + 1;
	(*output) += 4;
}


static inline void transfer_YUVA8888_to_VYU888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v, a, anti_a;
	a = input[3];
	anti_a = 0xff - a;
	y = ((uint32_t)input[0] * a) / 0xff;
	u = ((uint32_t)input[1] * a + 0x80 * anti_a) / 0xff;
	v = ((uint32_t)input[2] * a + 0x80 * anti_a) / 0xff;
	
	(*output)[0] = v;
	(*output)[1] = y;
	(*output)[2] = u;
	(*output) += 3;
}


static inline void transfer_YUVA8888_to_YUVA8888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[0];
	(*output)[1] = input[1];
	(*output)[2] = input[2];
	(*output)[3] = input[3];
	(*output) += 4;
}

static inline void transfer_YUVA8888_to_UYVA8888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[1];
	(*output)[1] = input[0];
	(*output)[2] = input[2];
	(*output)[3] = input[3];
	(*output) += 4;
}

static inline void transfer_YUVA8888_to_YUV101010(unsigned char *(*output), unsigned char *input)
{
	uint16_t y_i = ((uint16_t)input[0] * input[3]) + 0x1fe;
	uint16_t u_i = ((uint16_t)input[1] * input[3]) + 0x1fe;
	uint16_t v_i = ((uint16_t)input[2] * input[3]) + 0x1fe;
	WRITE_YUV101010(y_i, u_i, v_i);
}


static inline void transfer_YUVA8888_to_YUV420P_YUV422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column)
{
	int opacity = input[3];
	int transparency = 0xff - opacity;

	output_y[output_column] =     ((input[0] * opacity) >> 8) + 1;
	output_u[output_column / 2] = ((input[1] * opacity + 0x80 * transparency) >> 8) + 1;
	output_v[output_column / 2] = ((input[2] * opacity + 0x80 * transparency) >> 8) + 1;
}

static inline void transfer_YUVA8888_to_YUV444P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column)
{
	int opacity = input[3];
	int transparency = 0xff - opacity;

	output_y[output_column] =     ((input[0] * opacity) >> 8) + 1;
	output_u[output_column] = ((input[1] * opacity + 0x80 * transparency) >> 8) + 1;
	output_v[output_column] = ((input[2] * opacity + 0x80 * transparency) >> 8) + 1;
}

static inline void transfer_YUVA8888_to_YUV422(unsigned char *(*output), 
	unsigned char *input,
	int j)
{
	int opacity = input[3];
	int transparency = 0xff - opacity;
// Store U and V for even pixels only
	if(!(j & 1))
	{
		(*output)[0] = ((input[0] * opacity) >> 8) + 1;
		(*output)[1] = ((input[1] * opacity + 0x80 * transparency) >> 8) + 1;
		(*output)[3] = ((input[2] * opacity + 0x80 * transparency) >> 8) + 1;
	}
	else
// Store Y and advance output for odd pixels only
	{
		(*output)[2] = ((input[0] * opacity) >> 8) + 1;
		(*output) += 4;
	}
}











// ********************************* YUV101010 -> *****************************

#define READ_YUV101010 \
	uint64_t y, u, v; \
	uint32_t input_i = input[0] | \
		(input[1] << 8) | \
		(input[2] << 16) | \
		(input[3] << 24); \
 \
	y = (input_i & 0xffc00000) >> 16; \
	u = (input_i & 0x3ff000) >> 6; \
	v = (input_i & 0xffc) << 4;

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









// ******************************** YUV161616 -> ******************************


static inline void transfer_YUV161616_to_RGB8(unsigned char *(*output), uint16_t *input)
{
	int y, u, v;
	int r, g, b;
	
	y = (((int)input[0]) << 8) | (input[0] >> 8);
	u = input[1] >> 8;
	v = input[2] >> 8;
	YUV_TO_RGB(y, u, v, r, g, b);

	*(*output) = (unsigned char)((r & 0xc0) +
			    			 ((g & 0xe0) >> 2) +
		 	    			 ((b & 0xe0) >> 5));
	(*output)++;
}

static inline void transfer_YUV161616_to_BGR565(unsigned char *(*output), uint16_t *input)
{
	int y, u, v;
	int r, g, b;
	
	y = (((int)input[0]) << 8) | (input[0] >> 8);
	u = input[1] >> 8;
	v = input[2] >> 8;
	YUV_TO_RGB(y, u, v, r, g, b);
	*(uint16_t*)(*output) = ((b & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((r & 0xf8) >> 3);
	(*output) += 2;
}

static inline void transfer_YUV161616_to_RGB565(unsigned char *(*output), uint16_t *input)
{
	int y, u, v;
	int r, g, b;
	
	y = (((int)input[0]) << 8) | (input[0] >> 8);
	u = input[1] >> 8;
	v = input[2] >> 8;
	YUV_TO_RGB(y, u, v, r, g, b);
	*(uint16_t*)(*output) = ((r & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((b & 0xf8) >> 3);
	(*output) += 2;
}

static inline void transfer_YUV161616_to_BGR888(unsigned char *(*output), uint16_t *input)
{
	int y, u, v;
	int r, g, b;
	
	y = (((int)input[0]) << 8) | (input[0] >> 8);
	u = input[1];
	v = input[2];
	YUV_TO_RGB16(y, u, v, r, g, b);

	(*output)[2] = r >> 8;
	(*output)[1] = g >> 8;
	(*output)[0] = b >> 8;
	(*output) += 3;
}

static inline void transfer_YUV161616_to_RGB888(unsigned char *(*output), uint16_t *input)
{
	int y, u, v;
	int r, g, b;
	
	y = (((int)input[0]) << 8) | (input[0] >> 8);
	u = input[1];
	v = input[2];
	YUV_TO_RGB16(y, u, v, r, g, b);

	(*output)[0] = r >> 8;
	(*output)[1] = g >> 8;
	(*output)[2] = b >> 8;
	(*output) += 3;
}

static inline void transfer_YUV161616_to_RGBA8888(unsigned char *(*output), uint16_t *input)
{
	int y, u, v;
	int r, g, b;
	
	y = (((int)input[0]) << 8) | (input[0] >> 8);
	u = input[1];
	v = input[2];
	YUV_TO_RGB16(y, u, v, r, g, b);

	(*output)[0] = r >> 8;
	(*output)[1] = g >> 8;
	(*output)[2] = b >> 8;
	(*output)[3] = 0xff;
	(*output) += 4;
}

static inline void transfer_YUV161616_to_BGR8888(unsigned char *(*output), uint16_t *input)
{
	int y, u, v;
	int r, g, b;

	y = (((int)input[0]) << 8) | (input[0] >> 8);
	u = input[1] >> 8;
	v = input[2] >> 8;
	YUV_TO_RGB(y, u, v, r, g, b);
	(*output)[2] = r;
	(*output)[1] = g;
	(*output)[0] = b;
	(*output) += 4;
}

static inline void transfer_YUV161616_to_YUV161616(uint16_t *(*output), uint16_t *input)
{
	(*output)[0] = input[0];
	(*output)[1] = input[1];
	(*output)[2] = input[2];
	(*output) += 3;
}

static inline void transfer_YUV161616_to_YUVA8888(unsigned char *(*output), uint16_t *input)
{
	(*output)[0] = input[0] >> 8;
	(*output)[1] = input[1] >> 8;
	(*output)[2] = input[2] >> 8;
	(*output)[3] = 255;
	(*output) += 4;
}


static inline void transfer_YUV161616_to_VYU888(unsigned char *(*output), uint16_t *input)
{
	(*output)[0] = input[2] >> 8;
	(*output)[1] = input[0] >> 8;
	(*output)[2] = input[1] >> 8;
	(*output) += 3;
}


static inline void transfer_YUV161616_to_UYVA8888(unsigned char *(*output), uint16_t *input)
{
	(*output)[0] = input[1] >> 8;
	(*output)[1] = input[0] >> 8;
	(*output)[2] = input[2] >> 8;
	(*output)[3] = input[3] >> 8;
	(*output) += 4;
}

static inline void transfer_YUV161616_to_YUV101010(unsigned char *(*output), uint16_t *input)
{
	uint16_t y_i = input[0];
	uint16_t u_i = input[1];
	uint16_t v_i = input[2];
	WRITE_YUV101010(y_i, u_i, v_i);
}

static inline void transfer_YUV161616_to_YUV420P_YUV422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	uint16_t *input,
	int output_column)
{
	output_y[output_column] = input[0] >> 8;
	output_u[output_column / 2] = input[1] >> 8;
	output_v[output_column / 2] = input[2] >> 8;
}

static inline void transfer_YUV161616_to_YUV444P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	uint16_t *input,
	int output_column)
{
	output_y[output_column] = input[0] >> 8;
	output_u[output_column] = input[1] >> 8;
	output_v[output_column] = input[2] >> 8;
}

static inline void transfer_YUV161616_to_YUV422(unsigned char *(*output), 
	uint16_t *input,
	int j)
{
// Store U and V for even pixels only
	if(!(j & 1))
	{
		(*output)[1] = input[1] >> 8;
		(*output)[3] = input[2] >> 8;
		(*output)[0] = input[0] >> 8;
	}
	else
// Store Y and advance output for odd pixels only
	{
		(*output)[2] = input[0] >> 8;
		(*output) += 4;
	}
}











// ******************************** YUVA16161616 -> ***************************




static inline void transfer_YUVA16161616_to_RGB8(unsigned char *(*output), uint16_t *input)
{
	int y, u, v, a;
	int r, g, b;
	
	a = input[3];
	y = (((int)input[0]) << 8) | (input[0] >> 8);
	u = input[1] >> 8;
	v = input[2] >> 8;
	YUV_TO_RGB(y, u, v, r, g, b);
	
	r *= a;
	g *= a;
	b *= a;

	*(*output) = (unsigned char)(((r & 0xc000) >> 8) + 
				((g & 0xe000) >> 10) + 
				((b & 0xe000) >> 13));
	(*output)++;
}

static inline void transfer_YUVA16161616_to_BGR565(unsigned char *(*output), uint16_t *input)
{
	int y, u, v, a;
	int r, g, b;
	
	a = input[3] >> 8;
	y = (((int)input[0]) << 8) | (input[0] >> 8);
	u = input[1] >> 8;
	v = input[2] >> 8;
	YUV_TO_RGB(y, u, v, r, g, b);
		
	r *= a;
	g *= a;
	b *= a;

	*(uint16_t*)(*output) = (uint16_t)((b & 0xf800) + 
				((g & 0xfc00) >> 5) + 
				((r & 0xf800) >> 11));
	(*output) += 2;
}

static inline void transfer_YUVA16161616_to_RGB565(unsigned char *(*output), uint16_t *input)
{
	int y, u, v, a;
	int r, g, b;
	
	a = input[3] >> 8;
	y = (((int)input[0]) << 8) | (input[0] >> 8);
	u = input[1] >> 8;
	v = input[2] >> 8;
	YUV_TO_RGB(y, u, v, r, g, b);
		
	r *= a;
	g *= a;
	b *= a;

	*(uint16_t*)(*output) = (uint16_t)((r & 0xf800) + 
				((g & 0xfc00) >> 5) + 
				((b & 0xf800) >> 11));
	(*output) += 2;
}

static inline void transfer_YUVA16161616_to_BGR888(unsigned char *(*output), uint16_t *input)
{
	int y, u, v, a;
	int r, g, b;
	
	a = input[3];
	y = (((int)input[0]) << 8) | (input[0] >> 8);
	u = input[1];
	v = input[2];

	YUV_TO_RGB16(y, u, v, r, g, b);
		
	r *= a;
	g *= a;
	b *= a;

	(*output)[0] = b / 0xffff00;
	(*output)[1] = g / 0xffff00;
	(*output)[2] = r / 0xffff00;
	(*output) += 3;
}

static inline void transfer_YUVA16161616_to_RGB888(unsigned char *(*output), uint16_t *input)
{
	unsigned int y, u, v, a;
	unsigned int r, g, b;

	a = input[3];
	y = (((int)input[0]) << 8) | (input[0] >> 8);
	u = input[1];
	v = input[2];

	YUV_TO_RGB16(y, u, v, r, g, b);
	
	r *= a;
	g *= a;
	b *= a;
	r /= 0xffff00;
	g /= 0xffff00;
	b /= 0xffff00;

	RECLIP(r, 0, 0xff);
	RECLIP(g, 0, 0xff);
	RECLIP(b, 0, 0xff);

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output) += 3;
}

static inline void transfer_YUVA16161616_to_RGBA8888(unsigned char *(*output), uint16_t *input)
{
	unsigned int y, u, v;
	unsigned int r, g, b;
	
	y = (((int)input[0]) << 8) | (input[0] >> 8);
	u = input[1];
	v = input[2];

	YUV_TO_RGB16(y, u, v, r, g, b);

	(*output)[0] = (r >> 8);
	(*output)[1] = (g >> 8);
	(*output)[2] = (b >> 8);
	(*output)[3] = input[3];
	(*output) += 4;
}

static inline void transfer_YUVA16161616_to_BGR8888(unsigned char *(*output), uint16_t *input)
{
	int y, u, v, a;
	int64_t r, g, b;

	a = input[3];
	y = (((int)input[0]) << 8) | (input[0] >> 8);
	u = input[1] >> 8;
	v = input[2] >> 8;

	YUV_TO_RGB(y, u, v, r, g, b);

	r *= a;
	g *= a;
	b *= a;
	b /= 0xffff;
	g /= 0xffff;
	r /= 0xffff;

	(*output)[0] = b;
	(*output)[1] = g;
	(*output)[2] = r;
	(*output) += 4;
}


static inline void transfer_YUVA16161616_to_VYU888(unsigned char *(*output), uint16_t *input)
{
	int y, u, v, a, anti_a;
	a = input[3];
	anti_a = 0xffff - a;
	y = ((uint32_t)input[0] * a) / 0xffff00;
	u = ((uint32_t)input[1] * a + 0x8000 * anti_a) / 0xffff00;
	v = ((uint32_t)input[2] * a + 0x8000 * anti_a) / 0xffff00;

	(*output)[0] = v;
	(*output)[1] = y;
	(*output)[2] = u;
	(*output) += 3;
}


static inline void transfer_YUVA16161616_to_YUVA16161616(uint16_t *(*output), uint16_t *input)
{
	(*output)[0] = input[0];
	(*output)[1] = input[1];
	(*output)[2] = input[2];
	(*output)[3] = input[3];
	(*output) += 4;
}

static inline void transfer_YUVA16161616_to_UYVA8888(unsigned char *(*output), uint16_t *input)
{
	(*output)[0] = input[1] >> 8;
	(*output)[1] = input[0] >> 8;
	(*output)[2] = input[2] >> 8;
	(*output)[3] = input[3] >> 8;
	(*output) += 4;
}


static inline void transfer_YUVA16161616_to_YUV101010(unsigned char *(*output), uint16_t *input)
{
	int64_t opacity = input[3];
	int64_t transparency = 0xffff - opacity;
	uint16_t y_i = ((int64_t)input[0] * opacity + (int64_t)0x8000 * transparency) / 0xffff;
	uint16_t u_i = ((int64_t)input[1] * opacity + (int64_t)0x8000 * transparency) / 0xffff;
	uint16_t v_i = ((int64_t)input[2] * opacity + (int64_t)0x8000 * transparency) / 0xffff;
	WRITE_YUV101010(y_i, u_i, v_i);
}

static inline void transfer_YUVA16161616_to_YUV420P_YUV422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	uint16_t *input,
	int output_column)
{
	int64_t opacity = input[3];
	int64_t transparency = 0xffff - opacity;

	output_y[output_column] =     ((int64_t)input[0] * opacity) / 0xffff00;
	output_u[output_column / 2] = ((int64_t)input[1] * opacity + 0x8000 * transparency) / 0xffff00;
	output_v[output_column / 2] = ((int64_t)input[2] * opacity + 0x8000 * transparency) / 0xffff00;
}

static inline void transfer_YUVA16161616_to_YUV444P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	uint16_t *input,
	int output_column)
{
	int64_t opacity = input[3];
	int64_t transparency = 0xffff - opacity;

	output_y[output_column] = ((int64_t)input[0] * opacity) / 0xffff00;
	output_u[output_column] = ((int64_t)input[1] * opacity + 0x8000 * transparency) / 0xffff00;
	output_v[output_column] = ((int64_t)input[2] * opacity + 0x8000 * transparency) / 0xffff00;
}

static inline void transfer_YUVA16161616_to_YUV422(unsigned char *(*output), 
	uint16_t *input,
	int j)
{
	int64_t opacity = input[3];
	int64_t transparency = 0xffff - opacity;

// Store U and V for even pixels only
	if(!(j & 1))
	{
		(*output)[0] = ((int64_t)input[0] * opacity) / 0xffff00;
		(*output)[1] = ((int64_t)input[1] * opacity + 0x8000 * transparency) / 0xffff00;
		(*output)[3] = ((int64_t)input[2] * opacity + 0x8000 * transparency) / 0xffff00;
	}
	else
// Store Y and advance output for odd pixels only
	{
		(*output)[2] = (input[0] * opacity) / 0xffff00;
		(*output) += 4;
	}
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
	
	y = ((int)input[1]) << 16;
	u = input[2] << 8;
	v = input[0] << 8;
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

	y = ((int)input[1]) << 16;
	u = input[2] << 8;
	v = input[0] << 8;
	YUV_TO_RGB16(y, u, v, r, g, b);

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output)[3] = 0xffff;
	(*output) += 3;
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
	u = input[0] << 8;
	v = input[2] << 8;
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
	u = input[0] << 8;
	v = input[2] << 8;
	YUV_TO_RGB16(y, u, v, r, g, b);

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output)[3] = input[3] << 8;
	(*output) += 4;
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















// ******************************** YUV422P -> ********************************

static inline void transfer_YUV422P_to_RGB8(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	int y, u, v;
	int r, g, b;
	
	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	*(*output) = (unsigned char)((r & 0xc0) +
			    			 ((g & 0xe0) >> 2) +
		 	    			 ((b & 0xe0) >> 5));
	(*output)++;
}

static inline void transfer_YUV422P_to_BGR565(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	int y, u, v;
	int r, g, b;
	
	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	*(uint16_t*)(*output) = ((b & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((r & 0xf8) >> 3);
	(*output) += 2;
}

static inline void transfer_YUV422P_to_RGB565(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	int y, u, v;
	int r, g, b;
	
	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	*(uint16_t*)(*output) = ((r & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((b & 0xf8) >> 3);
	(*output) += 2;
}

static inline void transfer_YUV422P_to_BGR888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	int y, u, v;
	int r, g, b;
	
	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = b;
	(*output)[1] = g;
	(*output)[2] = r;
	(*output) += 3;
}

static inline void transfer_YUV422P_to_BGR8888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	int y, u, v;
	int r, g, b;

	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = b;
	(*output)[1] = g;
	(*output)[2] = r;
	(*output) += 4;
}

static inline void transfer_YUV422P_to_RGB888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
// Signedness is important
	int y, u, v;
	int r, g, b;

	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output) += 3;
}

static inline void transfer_YUV422P_to_ARGB8888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
// Signedness is important
	int y, u, v;
	int r, g, b;

	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = 0xff;
	(*output)[1] = r;
	(*output)[2] = g;
	(*output)[3] = b;
	(*output) += 4;
}

static inline void transfer_YUV422P_to_ABGR8888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
// Signedness is important
	int y, u, v;
	int r, g, b;

	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = 0xff;
	(*output)[3] = r;
	(*output)[2] = g;
	(*output)[1] = b;
	(*output) += 4;
}

static inline void transfer_YUV422P_to_RGBA8888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
// Signedness is important
	int y, u, v;
	int r, g, b;

	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output)[3] = 0xff;
	(*output) += 4;
}

static inline void transfer_YUV422P_to_RGB161616(uint16_t *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
// Signedness is important
	int y, u, v;
	int r, g, b;
	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = (*input_u << 8) | *input_u;
	v = (*input_v << 8) | *input_v;
	YUV_TO_RGB16(y, u, v, r, g, b)

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;

	(*output) += 3;
}


static inline void transfer_YUV422P_to_RGBA16161616(uint16_t *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
// Signedness is important
	int y, u, v;
	int r, g, b;
	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = (*input_u << 8) | *input_u;
	v = (*input_v << 8) | *input_v;
	YUV_TO_RGB16(y, u, v, r, g, b)

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output)[3] = 0xffff;

	(*output) += 4;
}



static inline void transfer_YUV422P_to_YUV888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	(*output)[0] = *input_y;
	(*output)[1] = *input_u;
	(*output)[2] = *input_v;
	(*output) += 3;
}

static inline void transfer_YUV422P_to_YUV161616(uint16_t *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	(*output)[0] = (*input_y << 8) | *input_y;
	(*output)[1] = (*input_u << 8) | *input_u;
	(*output)[2] = (*input_v << 8) | *input_v;
	(*output) += 3;
}

static inline void transfer_YUV422P_to_YUVA8888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	(*output)[0] = *input_y;
	(*output)[1] = *input_u;
	(*output)[2] = *input_v;
	(*output)[3] = 0xff;
	(*output) += 4;
}

static inline void transfer_YUV422P_to_YUVA16161616(uint16_t *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	(*output)[0] = (((uint16_t)*input_y) << 8) | *input_y;
	(*output)[1] = (((uint16_t)*input_u) << 8) | *input_u;
	(*output)[2] = (((uint16_t)*input_v) << 8) | *input_v;

	(*output)[3] = 0xffff;
	(*output) += 4;
}

static inline void transfer_YUV422P_to_YUV420P(unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v,
	unsigned char *output_y,
	unsigned char *output_u,
	unsigned char *output_v,
	int j)
{
	output_y[j] = *input_y;
	output_u[j / 2] = *input_u;
	output_v[j / 2] = *input_v;
}

static inline void transfer_YUV422P_to_YUV444P(unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v,
	unsigned char *output_y,
	unsigned char *output_u,
	unsigned char *output_v,
	int j)
{
	output_y[j] = *input_y;
	output_u[j] = *input_u;
	output_v[j] = *input_v;
}

static inline void transfer_YUV422P_to_YUV422(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v,
	int j)
{
// Store U and V for even pixels only
	if(!(j & 1))
	{
		(*output)[1] = *input_u;
		(*output)[3] = *input_v;
		(*output)[0] = *input_y;
	}
	else
// Store Y and advance output for odd pixels only
	{
		(*output)[2] = *input_y;
		(*output) += 4;
	}
}















// ******************************** YUV444P -> ********************************

static inline void transfer_YUV444P_to_YUV444P(unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v,
	unsigned char *output_y,
	unsigned char *output_u,
	unsigned char *output_v,
	int j)
{
	output_y[j] = *input_y;
	output_u[j] = *input_u;
	output_v[j] = *input_v;
}










// ******************************** YUV422 -> *********************************

static inline void transfer_YUV422_to_RGB8(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (int)(input[0]) << 16;
	else
// Odd pixel
		y = (int)(input[2]) << 16;

	u = input[1];
	v = input[3];
	YUV_TO_RGB(y, u, v, r, g, b)

	*(*output) = (unsigned char)((r & 0xc0) +
			    			 ((g & 0xe0) >> 2) +
		 	    			 ((b & 0xe0) >> 5));
	(*output)++;
}

static inline void transfer_YUV422_to_BGR565(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (int)(input[0]) << 16;
	else
// Odd pixel
		y = (int)(input[2]) << 16;
	u = input[1];
	v = input[3];
	YUV_TO_RGB(y, u, v, r, g, b)

	*(uint16_t*)(*output) = ((b & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((r & 0xf8) >> 3);
	(*output) += 2;
}

static inline void transfer_YUV422_to_RGB565(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (int)(input[0]) << 16;
	else
// Odd pixel
		y = (int)(input[2]) << 16;
	u = input[1];
	v = input[3];
	YUV_TO_RGB(y, u, v, r, g, b)

	*(uint16_t*)(*output) = ((r & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((b & 0xf8) >> 3);
	(*output) += 2;
}

static inline void transfer_YUV422_to_BGR888(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (int)(input[0]) << 16;
	else
// Odd pixel
		y = (int)(input[2]) << 16;
	u = input[1];
	v = input[3];
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = b;
	(*output)[1] = g;
	(*output)[2] = r;
	(*output) += 3;
}

static inline void transfer_YUV422_to_RGB888(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (input[0] << 16) | (input[0] << 8) | input[0];
	else
// Odd pixel
		y = (input[2] << 16) | (input[2] << 8) | input[2];
	u = input[1];
	v = input[3];
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output) += 3;
}

static inline void transfer_YUV422_to_RGBA8888(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (input[0] << 16) | (input[0] << 8) | input[0];
	else
// Odd pixel
		y = (input[2] << 16) | (input[2] << 8) | input[2];
	u = input[1];
	v = input[3];
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output)[3] = 0xff;
	(*output) += 4;
}

static inline void transfer_YUV422_to_RGB161616(uint16_t *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (input[0] << 16) | (input[0] << 8) | input[0];
	else
// Odd pixel
		y = (input[2] << 16) | (input[2] << 8) | input[2];
	u = (input[1] << 8) | input[1];
	v = (input[3] << 8) | input[3];
	YUV_TO_RGB16(y, u, v, r, g, b)

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output) += 3;
}

static inline void transfer_YUV422_to_RGBA16161616(uint16_t *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (input[0] << 16) | (input[0] << 8) | input[0];
	else
// Odd pixel
		y = (input[2] << 16) | (input[2] << 8) | input[2];
	u = (input[1] << 8) | input[1];
	v = (input[3] << 8) | input[3];
	YUV_TO_RGB16(y, u, v, r, g, b)

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output)[3] = 0xffff;
	(*output) += 4;
}

static inline void transfer_YUV422_to_YUV888(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
// Even pixel
	if(!(column & 1))
		(*output)[0] = input[0];
	else
// Odd pixel
		(*output)[0] = input[2];

	(*output)[1] = input[1];
	(*output)[2] = input[3];
	(*output) += 3;
}

static inline void transfer_YUV422_to_YUVA8888(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
// Even pixel
	if(!(column & 1))
		(*output)[0] = input[0];
	else
// Odd pixel
		(*output)[0] = input[2];

	(*output)[1] = input[1];
	(*output)[2] = input[3];
	(*output)[3] = 255;
	(*output) += 4;
}

static inline void transfer_YUV422_to_YUV161616(uint16_t *(*output), 
	unsigned char *input, 
	int column)
{
// Even pixel
	if(!(column & 1))
		(*output)[0] = (input[0] << 8) | input[0];
	else
// Odd pixel
		(*output)[0] = (input[2] << 8) | input[2];

	(*output)[1] = (input[1] << 8) | input[1];
	(*output)[2] = (input[3] << 8) | input[3];
	(*output) += 3;
}

static inline void transfer_YUV422_to_YUVA16161616(uint16_t *(*output), 
	unsigned char *input, 
	int column)
{
// Even pixel
	if(!(column & 1))
		(*output)[0] = (input[0] << 8) | input[0];
	else
// Odd pixel
		(*output)[0] = (input[2] << 8) | input[2];

	(*output)[1] = (input[1] << 8) | input[1];
	(*output)[2] = (input[3] << 8) | input[3];
	(*output)[3] = 0xffff;
	(*output) += 4;
}

static inline void transfer_YUV422_to_BGR8888(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (int)(input[0]) << 16;
	else
// Odd pixel
		y = (int)(input[2]) << 16;
	u = input[1];
	v = input[3];

	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = b;
	(*output)[1] = g;
	(*output)[2] = r;
	(*output) += 4;
}


static inline void transfer_YUV422_to_YUV422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column)
{
// Store U and V for even pixels only
	if(!(output_column & 1))
	{
		output_y[output_column] = input[0];
		output_u[output_column / 2] = input[1];
		output_v[output_column / 2] = input[3];
	}
	else
// Store Y and advance output for odd pixels only
	{
		output_y[output_column] = input[2];
	}
}

static inline void transfer_YUV422_to_YUV420P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column,
	int output_row)
{
// Even column
	if(!(output_column & 1))
	{
		output_y[output_column] = input[0];
// Store U and V for even columns and even rows only
		if(!(output_row & 1))
		{
			output_u[output_column / 2] = input[1];
			output_v[output_column / 2] = input[3];
		}
	}
	else
// Odd column
	{
		output_y[output_column] = input[2];
	}
}

static inline void transfer_YUV422_to_YUV422(unsigned char *(*output), 
	unsigned char *input,
	int j)
{
// Store U and V for even pixels only
	if(!(j & 1))
	{
		(*output)[0] = input[0];
		(*output)[1] = input[1];
		(*output)[3] = input[3];
	}
	else
// Store Y and advance output for odd pixels only
	{
		(*output)[2] = input[2];
		(*output) += 4;
	}
}








// ******************************** Loops *************************************

#define TRANSFER_FRAME_HEAD \
	for(i = 0; i < out_h; i++) \
	{ \
		unsigned char *output_row = output_rows[i + out_y] + out_x * out_pixelsize; \
		unsigned char *input_row = input_rows[row_table[i]]; \
		int bit_counter = 7; \
		for(j = 0; j < out_w; j++) \
		{

#define TRANSFER_FRAME_TAIL \
		} \
	}

#define TRANSFER_YUV420P_OUT_HEAD \
	for(i = 0; i < out_h; i++) \
	{ \
		unsigned char *input_row = input_rows[row_table[i]]; \
		unsigned char *output_y = out_y_plane + i * total_out_w + out_x; \
		unsigned char *output_u = out_u_plane + i / 2 * total_out_w / 2 + out_x / 2; \
		unsigned char *output_v = out_v_plane + i / 2 * total_out_w / 2 + out_x / 2; \
		for(j = 0; j < out_w; j++) \
		{

#define TRANSFER_YUV422P_OUT_HEAD \
	for(i = 0; i < out_h; i++) \
	{ \
		unsigned char *input_row = input_rows[row_table[i]]; \
		unsigned char *output_y = out_y_plane + i * total_out_w + out_x; \
		unsigned char *output_u = out_u_plane + i * total_out_w / 2 + out_x / 2; \
		unsigned char *output_v = out_v_plane + i * total_out_w / 2 + out_x / 2; \
		for(j = 0; j < out_w; j++) \
		{

#define TRANSFER_YUV444P_OUT_HEAD \
	for(i = 0; i < out_h; i++) \
	{ \
		unsigned char *input_row = input_rows[row_table[i]]; \
		unsigned char *output_y = out_y_plane + i * total_out_w + out_x; \
		unsigned char *output_u = out_u_plane + i * total_out_w + out_x; \
		unsigned char *output_v = out_v_plane + i * total_out_w + out_x; \
		for(j = 0; j < out_w; j++) \
		{

#define TRANSFER_YUV420P_IN_HEAD \
	for(i = 0; i < out_h; i++) \
	{ \
		unsigned char *output_row = output_rows[i + out_y] + out_x * out_pixelsize; \
		unsigned char *input_y = in_y_plane + row_table[i] * total_in_w; \
		unsigned char *input_u = in_u_plane + (row_table[i] / 2) * (total_in_w / 2); \
		unsigned char *input_v = in_v_plane + (row_table[i] / 2) * (total_in_w / 2); \
		for(j = 0; j < out_w; j++) \
		{


#define TRANSFER_YUV422P_IN_HEAD \
	for(i = 0; i < out_h; i++) \
	{ \
		unsigned char *output_row = output_rows[i + out_y] + out_x * out_pixelsize; \
		unsigned char *input_y = in_y_plane + row_table[i] * total_in_w; \
		unsigned char *input_u = in_u_plane + row_table[i] * (total_in_w / 2); \
		unsigned char *input_v = in_v_plane + row_table[i] * (total_in_w / 2); \
		for(j = 0; j < out_w; j++) \
		{

#define TRANSFER_YUV444P_IN_HEAD \
	for(i = 0; i < out_h; i++) \
	{ \
		unsigned char *output_row = output_rows[i + out_y] + out_x * out_pixelsize; \
		unsigned char *input_y = in_y_plane + row_table[i] * total_in_w; \
		unsigned char *input_u = in_u_plane + row_table[i] * total_in_w; \
		unsigned char *input_v = in_v_plane + row_table[i] * total_in_w; \
		for(j = 0; j < out_w; j++) \
		{


#define TRANSFER_YUV422_IN_HEAD \
	for(i = 0; i < out_h; i++) \
	{ \
		unsigned char *output_row = output_rows[i + out_y] + ((out_x * out_pixelsize) & 0xfffffffc); \
		unsigned char *input_y = in_y_plane + row_table[i] * total_in_w; \
		unsigned char *input_u = in_u_plane + row_table[i] * (total_in_w / 2); \
		unsigned char *input_v = in_v_plane + row_table[i] * (total_in_w / 2); \
		for(j = 0; j < out_w; j++) \
		{





// ******************************** Permutation *******************************



#define PERMUTATION_ARGS \
	unsigned char **output_rows,  \
	unsigned char **input_rows, \
	unsigned char *out_y_plane, \
	unsigned char *out_u_plane, \
	unsigned char *out_v_plane, \
	unsigned char *in_y_plane, \
	unsigned char *in_u_plane, \
	unsigned char *in_v_plane, \
	int in_x,  \
	int in_y,  \
	int in_w,  \
	int in_h, \
	int out_x,  \
	int out_y,  \
	int out_w,  \
	int out_h, \
	int in_colormodel,  \
	int out_colormodel, \
	int bg_color, \
	int total_in_w, \
	int total_out_w, \
	int scale, \
	int out_pixelsize, \
	int in_pixelsize, \
	int *row_table, \
	int *column_table, \
	int bg_r, \
	int bg_g, \
	int bg_b

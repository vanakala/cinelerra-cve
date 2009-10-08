
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

#ifndef PLUGINCOLORS_H
#define PLUGINCOLORS_H

// Duplicate filename in guicast

#include "clip.h"
#include "vframe.inc"

#include <stdint.h>

// Compression coefficients straight out of jpeglib
#define R_TO_Y    0.29900
#define G_TO_Y    0.58700
#define B_TO_Y    0.11400

#define R_TO_U    -0.16874
#define G_TO_U    -0.33126
#define B_TO_U    0.50000

#define R_TO_V    0.50000
#define G_TO_V    -0.41869
#define B_TO_V    -0.08131

// Decompression coefficients straight out of jpeglib
#define V_TO_R    1.40200
#define V_TO_G    -0.71414

#define U_TO_G    -0.34414
#define U_TO_B    1.77200


class YUV
{
public:
	YUV();
	~YUV();

	inline void rgb_to_yuv_8(int &y, int &u, int &v)
	{
		int r = y;
		int g = u;
		int b = v;
		y = (rtoy_tab_8[r] + gtoy_tab_8[g] + btoy_tab_8[b]) >> 8;
		u = (rtou_tab_8[r] + gtou_tab_8[g] + btou_tab_8[b]) >> 8;
		v = (rtov_tab_8[r] + gtov_tab_8[g] + btov_tab_8[b]) >> 8;
	};

	inline void rgb_to_yuv_8(int r, int g, int b, int &y, int &u, int &v)
	{
		y = (rtoy_tab_8[r] + gtoy_tab_8[g] + btoy_tab_8[b]) >> 8;
		u = (rtou_tab_8[r] + gtou_tab_8[g] + btou_tab_8[b]) >> 8;
		v = (rtov_tab_8[r] + gtov_tab_8[g] + btov_tab_8[b]) >> 8;
	};

	inline void rgb_to_yuv_8(int r, int g, int b, unsigned char &y, unsigned char &u, unsigned char &v)
	{
		y = (rtoy_tab_8[r] + gtoy_tab_8[g] + btoy_tab_8[b]) >> 8;
		u = (rtou_tab_8[r] + gtou_tab_8[g] + btou_tab_8[b]) >> 8;
		v = (rtov_tab_8[r] + gtov_tab_8[g] + btov_tab_8[b]) >> 8;
	};

	static inline void rgb_to_yuv_f(float r, float g, float b, float &y, float &u, float &v)
	{
		y = r * R_TO_Y + g * G_TO_Y + b * B_TO_Y;
		u = r * R_TO_U + g * G_TO_U + b * B_TO_U;
		v = r * R_TO_V + g * G_TO_V + b * B_TO_V;
	};

	inline void rgb_to_yuv_16(int r, int g, int b, int &y, int &u, int &v)
	{
		y = (rtoy_tab_16[r] + gtoy_tab_16[g] + btoy_tab_16[b]) >> 8;
		u = (rtou_tab_16[r] + gtou_tab_16[g] + btou_tab_16[b]) >> 8;
		v = (rtov_tab_16[r] + gtov_tab_16[g] + btov_tab_16[b]) >> 8;
	};

// For easier programming.  Doesn't do anything.
	inline void rgb_to_yuv_8(float r, float g, float b, float &y, float &u, float &v)
	{
	};

	inline void rgb_to_yuv_16(float r, float g, float b, float &y, float &u, float &v)
	{
	};

	static inline void rgb_to_yuv_f(int r, int g, int b, int &y, int &u, int &v)
	{
	};

	inline void yuv_to_rgb_8(int &r, int &g, int &b)
	{
		int y = r;
		int u = g;
		int v = b;
		y = (y << 8) | y;
		r = (y + vtor_tab_8[v]) >> 8;
		g = (y + utog_tab_8[u] + vtog_tab_8[v]) >> 8;
		b = (y + utob_tab_8[u]) >> 8;

		CLAMP(r, 0, 0xff);
		CLAMP(g, 0, 0xff);
		CLAMP(b, 0, 0xff);
	};
	inline void yuv_to_rgb_8(int &r, int &g, int &b, int y, int u, int v)
	{
		y = (y << 8) | y;
		r = (y + vtor_tab_8[v]) >> 8;
		g = (y + utog_tab_8[u] + vtog_tab_8[v]) >> 8;
		b = (y + utob_tab_8[u]) >> 8;

		CLAMP(r, 0, 0xff);
		CLAMP(g, 0, 0xff);
		CLAMP(b, 0, 0xff);
	};

	static inline void yuv_to_rgb_f(float &r, float &g, float &b, float y, float u, float v)
	{
		r = y + V_TO_R * v;
		g = y + U_TO_G * u + V_TO_G * v;
		b = y + U_TO_B * u;
	};

	inline void rgb_to_yuv_16(int r, int g, int b, uint16_t &y, uint16_t &u, uint16_t &v)
	{
		y = (rtoy_tab_16[r] + gtoy_tab_16[g] + btoy_tab_16[b]) >> 8;
		u = (rtou_tab_16[r] + gtou_tab_16[g] + btou_tab_16[b]) >> 8;
		v = (rtov_tab_16[r] + gtov_tab_16[g] + btov_tab_16[b]) >> 8;
	};

	inline void yuv_to_rgb_16(int &r, int &g, int &b, int y, int u, int v)
	{
		y = (y << 8) | y;
		r = (y + vtor_tab_16[v]) >> 8;
		g = (y + utog_tab_16[u] + vtog_tab_16[v]) >> 8;
		b = (y + utob_tab_16[u]) >> 8;

		CLAMP(r, 0, 0xffff);
		CLAMP(g, 0, 0xffff);
		CLAMP(b, 0, 0xffff);
	};

// For easier programming.  Doesn't do anything.
	inline void yuv_to_rgb_8(float &r, float &g, float &b, float y, float u, float v)
	{
	};

// For easier programming.  Doesn't do anything.
	inline void yuv_to_rgb_16(float &r, float &g, float &b, float y, float u, float v)
	{
	};

	static inline void yuv_to_rgb_f(int &r, int &g, int &b, int y, int u, int v)
	{
	};

private:
	int rtoy_tab_8[0x100], gtoy_tab_8[0x100], btoy_tab_8[0x100];
	int rtou_tab_8[0x100], gtou_tab_8[0x100], btou_tab_8[0x100];
	int rtov_tab_8[0x100], gtov_tab_8[0x100], btov_tab_8[0x100];

	int vtor_tab_8[0x100], vtog_tab_8[0x100];
	int utog_tab_8[0x100], utob_tab_8[0x100];
	int *vtor_8, *vtog_8, *utog_8, *utob_8;

	int rtoy_tab_16[0x10000], gtoy_tab_16[0x10000], btoy_tab_16[0x10000];
	int rtou_tab_16[0x10000], gtou_tab_16[0x10000], btou_tab_16[0x10000];
	int rtov_tab_16[0x10000], gtov_tab_16[0x10000], btov_tab_16[0x10000];

	int vtor_tab_16[0x10000], vtog_tab_16[0x10000];
	int utog_tab_16[0x10000], utob_tab_16[0x10000];
	int *vtor_16, *vtog_16, *utog_16, *utob_16;
};














class HSV
{
public:
	HSV();
    ~HSV();

// All units are 0 - 1
	static int rgb_to_hsv(float r, float g, float b, float &h, float &s, float &v);
	static int hsv_to_rgb(float &r, float &g, float &b, float h, float s, float v);

// YUV units are 0 - max.  HSV units are 0 - 1
	static int yuv_to_hsv(int y, int u, int v, float &h, float &s, float &va, int max);
	static int hsv_to_yuv(int &y, int &u, int &v, float h, float s, float va, int max);
// Dummies for macros
	static int yuv_to_hsv(float y, float u, float v, float &h, float &s, float &va, float max) { return 0; };
	static int hsv_to_yuv(float &y, float &u, float &v, float h, float s, float va, float max) { return 0; };
	static YUV yuv_static;
};


#endif

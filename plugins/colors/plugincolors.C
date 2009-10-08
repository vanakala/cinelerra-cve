
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

#include "plugincolors.h"

#include <stdio.h>

HSV::HSV()
{
}


HSV::~HSV()
{
}

YUV HSV::yuv_static;

int HSV::rgb_to_hsv(float r, float g, float b, float &h, float &s, float &v)
{
    int i;
	float min, max, delta;
	float f, p, q, t;
	min = ((r < g) ? r : g) < b ? ((r < g) ? r : g) : b;
	max = ((r > g) ? r : g) > b ? ((r > g) ? r : g) : b;
	v = max; 

	delta = max - min;

	if(max != 0 && delta != 0)
    {
	    s = delta / max;               // s

		if(r == max)
        	h = (g - b) / delta;         // between yellow & magenta
		else 
		if(g == max)
        	h = 2 + (b - r) / delta;     // between cyan & yellow
		else
        	h = 4 + (r - g) / delta;     // between magenta & cyan

		h *= 60;                               // degrees
		if(h < 0)
        	h += 360;
	}
	else 
	{
        // r = g = b = 0                // s = 0, v is undefined
        s = 0;
        h = -1;
	}
	
	return 0;
}

int HSV::hsv_to_rgb(float &r, float &g, float &b, float h, float s, float v)
{
    int i;
	float min, max, delta;
	float f, p, q, t;
    if(s == 0) 
	{
        // achromatic (grey)
        r = g = b = v;
        return 0;
    }

    h /= 60;                        // sector 0 to 5
    i = (int)h;
    f = h - i;                      // factorial part of h
    p = v * (1 - s);
    q = v * (1 - s * f);
    t = v * (1 - s * (1 - f));

    switch(i) 
	{
        case 0:
            r = v;
            g = t;
            b = p;
            break;
        case 1:
            r = q;
            g = v;
            b = p;
            break;
        case 2:
            r = p;
            g = v;
            b = t;
            break;
        case 3:
            r = p;
            g = q;
            b = v;
            break;
        case 4:
            r = t;
            g = p;
            b = v;
            break;
        default:                // case 5:
            r = v;
            g = p;
            b = q;
            break;
    }
	return 0;
}

int HSV::yuv_to_hsv(int y, int u, int v, float &h, float &s, float &va, int max)
{
	float r, g, b;
	int r_i, g_i, b_i;

	if(max == 0xffff)
	{
		yuv_static.yuv_to_rgb_16(r_i, g_i, b_i, y, u, v);
	}
	else
	{
		yuv_static.yuv_to_rgb_8(r_i, g_i, b_i, y, u, v);
	}
	r = (float)r_i / max;
	g = (float)g_i / max;
	b = (float)b_i / max;

	float h2, s2, v2;
	HSV::rgb_to_hsv(r, g, b, h2, s2, v2);
	h = h2;
	s = s2;
	va = v2;

	return 0;
}

int HSV::hsv_to_yuv(int &y, int &u, int &v, float h, float s, float va, int max)
{
	float r, g, b;
	int r_i, g_i, b_i;
	HSV::hsv_to_rgb(r, g, b, h, s, va);
	r = r * max + 0.5;
	g = g * max + 0.5;
	b = b * max + 0.5;
	r_i = (int)CLIP(r, 0, max);
	g_i = (int)CLIP(g, 0, max);
	b_i = (int)CLIP(b, 0, max);

	int y2, u2, v2;
	if(max == 0xffff)
		yuv_static.rgb_to_yuv_16(r_i, g_i, b_i, y2, u2, v2);
	else
		yuv_static.rgb_to_yuv_8(r_i, g_i, b_i, y2, u2, v2);
	y = y2;
	u = u2;
	v = v2;

	return 0;
}
























YUV::YUV()
{
	for(int i = 0; i < 0x100; i++)
	{
// compression
		rtoy_tab_8[i] = (int)(R_TO_Y * 0x100 * i);
		rtou_tab_8[i] = (int)(R_TO_U * 0x100 * i);
		rtov_tab_8[i] = (int)(R_TO_V * 0x100 * i);

		gtoy_tab_8[i] = (int)(G_TO_Y * 0x100 * i);
		gtou_tab_8[i] = (int)(G_TO_U * 0x100 * i);
		gtov_tab_8[i] = (int)(G_TO_V * 0x100 * i);

		btoy_tab_8[i] = (int)(B_TO_Y * 0x100 * i);
		btou_tab_8[i] = (int)(B_TO_U * 0x100 * i) + 0x8000;
		btov_tab_8[i] = (int)(B_TO_V * 0x100 * i) + 0x8000;
	}

	vtor_8 = &(vtor_tab_8[(0x100) / 2]);
	vtog_8 = &(vtog_tab_8[(0x100) / 2]);
	utog_8 = &(utog_tab_8[(0x100) / 2]);
	utob_8 = &(utob_tab_8[(0x100) / 2]);

	for(int i = (-0x100) / 2; i < (0x100) / 2; i++)
	{
// decompression
		vtor_8[i] = (int)(V_TO_R * 0x100 * i);
		vtog_8[i] = (int)(V_TO_G * 0x100 * i);

		utog_8[i] = (int)(U_TO_G * 0x100 * i);
		utob_8[i] = (int)(U_TO_B * 0x100 * i);
	}

	for(int i = 0; i < 0x10000; i++)
	{
// compression
		rtoy_tab_16[i] = (int)(R_TO_Y * 0x100 * i);
		rtou_tab_16[i] = (int)(R_TO_U * 0x100 * i);
		rtov_tab_16[i] = (int)(R_TO_V * 0x100 * i);

		gtoy_tab_16[i] = (int)(G_TO_Y * 0x100 * i);
		gtou_tab_16[i] = (int)(G_TO_U * 0x100 * i);
		gtov_tab_16[i] = (int)(G_TO_V * 0x100 * i);

		btoy_tab_16[i] = (int)(B_TO_Y * 0x100 * i);
		btou_tab_16[i] = (int)(B_TO_U * 0x100 * i) + 0x800000;
		btov_tab_16[i] = (int)(B_TO_V * 0x100 * i) + 0x800000;
	}

	vtor_16 = &(vtor_tab_16[(0x10000) / 2]);
	vtog_16 = &(vtog_tab_16[(0x10000) / 2]);
	utog_16 = &(utog_tab_16[(0x10000) / 2]);
	utob_16 = &(utob_tab_16[(0x10000) / 2]);

	for(int i = (-0x10000) / 2; i < (0x10000) / 2; i++)
	{
// decompression
		vtor_16[i] = (int)(V_TO_R * 0x100 * i);
		vtog_16[i] = (int)(V_TO_G * 0x100 * i);

		utog_16[i] = (int)(U_TO_G * 0x100 * i);
		utob_16[i] = (int)(U_TO_B * 0x100 * i);
	}
}

YUV::~YUV()
{
}

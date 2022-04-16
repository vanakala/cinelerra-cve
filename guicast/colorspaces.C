// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "datatype.h"
#include "colorspaces.h"
#include <math.h>

int ColorSpaces::rtoy_tab_8[0x100];
int ColorSpaces::gtoy_tab_8[0x100];
int ColorSpaces::btoy_tab_8[0x100];
int ColorSpaces::rtou_tab_8[0x100];
int ColorSpaces::gtou_tab_8[0x100];
int ColorSpaces::btou_tab_8[0x100];
int ColorSpaces::rtov_tab_8[0x100];
int ColorSpaces::gtov_tab_8[0x100];
int ColorSpaces::btov_tab_8[0x100];

int ColorSpaces::vtor_tab_8[0x100];
int ColorSpaces::vtog_tab_8[0x100];
int ColorSpaces::utog_tab_8[0x100];
int ColorSpaces::utob_tab_8[0x100];
int *ColorSpaces::vtor_8;
int *ColorSpaces::vtog_8;
int *ColorSpaces::utog_8;
int *ColorSpaces::utob_8;

int ColorSpaces::rtoy_tab_16[0x10000];
int ColorSpaces::gtoy_tab_16[0x10000];
int ColorSpaces::btoy_tab_16[0x10000];
int ColorSpaces::rtou_tab_16[0x10000];
int ColorSpaces::gtou_tab_16[0x10000];
int ColorSpaces::btou_tab_16[0x10000];
int ColorSpaces::rtov_tab_16[0x10000];
int ColorSpaces::gtov_tab_16[0x10000];
int ColorSpaces::btov_tab_16[0x10000];

int ColorSpaces::vtor_tab_16[0x10000];
int ColorSpaces::vtog_tab_16[0x10000];
int ColorSpaces::utog_tab_16[0x10000];
int ColorSpaces::utob_tab_16[0x10000];
int *ColorSpaces::vtor_16;
int *ColorSpaces::vtog_16;
int *ColorSpaces::utog_16;
int *ColorSpaces::utob_16;


ColorSpaces::ColorSpaces()
{
// initialize tables
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

void ColorSpaces::rgb_to_hsv(float r, float g, float b, float &h, float &s, float &v)
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
			h = (g - b) / delta;  // between yellow & magenta
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
}

#define PIXVAL_MAX 65535.0

void ColorSpaces::rgb_to_hsv(int red,  int green, int blue,
	int *hue, double *sat, double *val)
{
	double hf;
	double rf = red / PIXVAL_MAX;
	double gf = green / PIXVAL_MAX;
	double bf = blue / PIXVAL_MAX;
	double min = (((rf < gf) ? rf : gf) < bf ?
		((rf < gf) ? rf : gf) : bf);
	double max = (((rf > gf) ? rf : gf) > bf ?
		((rf > gf) ? rf : gf) : bf);
	double delta = max - min;

	*val = max;

	if(max > EPSILON && delta > EPSILON)
	{
		*sat = delta / max;

		if(rf == max)
			hf = (gf - bf) / delta;       // between yellow & magenta
		else if(gf == max)
			hf = 2 + (bf - rf) / delta;  // between cyan & yellow
		else
			hf = 4 + (rf - gf) / delta;  // between magenta & cyan
		hf *= 60;

		if(hf < 0)
			hf += 360;
		*hue = round(hf);
	}
	else
	{
		*hue = 0;
		*sat = 0;
	}
}

void ColorSpaces::hsv_to_rgb(float &r, float &g, float &b, float h, float s, float v)
{
	int i;
	float min, max, delta;
	float f, p, q, t;
	if(s == 0)
	{
// achromatic (grey)
		r = g = b = v;
		return;
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
}

void ColorSpaces::hsv_to_rgb(int *red, int *green, int *blue,
	int hue, double sat, double val)
{
	if(sat < EPSILON)
	{
// achromatic (grey)
		*red = *green = *blue = val * PIXVAL_MAX;
		return;
	}

	double hf = hue / 60.0;
	double frac = hf - trunc(hf);
	double p = val * (1 - sat);
	double q = val * (1 - sat * frac);
	double t = val * (1 - sat * (1 - frac));
	int sec = hf;

	switch(sec)
	{
	case 0:
		*red = val * PIXVAL_MAX;
		*green = t * PIXVAL_MAX;
		*blue = p * PIXVAL_MAX;
		break;
	case 1:
		*red = q * PIXVAL_MAX;
		*green = val * PIXVAL_MAX;
		*blue = p * PIXVAL_MAX;
		break;
	case 2:
		*red = p * PIXVAL_MAX;
		*green = val * PIXVAL_MAX;
		*blue = t * PIXVAL_MAX;
		break;
	case 3:
		*red = p * PIXVAL_MAX;
		*green = q * PIXVAL_MAX;
		*blue = val * PIXVAL_MAX;
		break;
	case 4:
		*red = t * PIXVAL_MAX;
		*green = p * PIXVAL_MAX;
		*blue = val * PIXVAL_MAX;
		break;
	default:                // case 5:
		*red = val * PIXVAL_MAX;
		*green = p * PIXVAL_MAX;
		*blue = q * PIXVAL_MAX;
		break;
	}
}

void ColorSpaces::yuv_to_hsv(int y, int u, int v, float &h, float &s, float &va, int max)
{
	float r, g, b;
	int r_i, g_i, b_i;

	if(max == 0xffff)
		yuv_to_rgb_16(r_i, g_i, b_i, y, u, v);
	else
		yuv_to_rgb_8(r_i, g_i, b_i, y, u, v);
	r = (float)r_i / max;
	g = (float)g_i / max;
	b = (float)b_i / max;

	float h2, s2, v2;
	rgb_to_hsv(r, g, b, h2, s2, v2);
	h = h2;
	s = s2;
	va = v2;
}

void ColorSpaces::hsv_to_yuv(int &y, int &u, int &v, float h, float s, float va, int max)
{
	float r, g, b;
	int r_i, g_i, b_i;

	hsv_to_rgb(r, g, b, h, s, va);
	r = r * max + 0.5;
	g = g * max + 0.5;
	b = b * max + 0.5;
	r_i = (int)CLIP(r, 0, max);
	g_i = (int)CLIP(g, 0, max);
	b_i = (int)CLIP(b, 0, max);

	int y2, u2, v2;
	if(max == 0xffff)
		rgb_to_yuv_16(r_i, g_i, b_i, y2, u2, v2);
	else
		rgb_to_yuv_8(r_i, g_i, b_i, y2, u2, v2);
	y = y2;
	u = u2;
	v = v2;
}

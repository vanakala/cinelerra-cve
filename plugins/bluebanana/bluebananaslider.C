/*
 * Cinelerra :: Blue Banana - color modification plugin for Cinelerra-CV
 * Copyright (C) 2012-2013 Monty <monty@xiph.org>
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

#include "bluebananaslider.h"

#undef CLAMP
#define CLAMP(x, y, z) ((x) < (y) ? (y) : ((x) > (z) ? (z) : (x)))

#define CW (get_h() / 2 + 2)
#define SW (get_h() * 2 / 5 + 1)
#define Z1 (get_h() / 7 + 2)
#define Z2 (get_h() * 2 / 5 + 1)
#define Z3 (get_h() - Z2 - 1)
#define Z4 (get_h() - Z1 - 1)
#define ZC ((Z3 + Z2) / 2)

#define ZS 3
#define ZL 1
#define ZR 3

#include <math.h>

static int scale_color(int color, float scale)
{
	int r = ((color >> 16) & 0xff) * scale;
	int g = ((color >> 8) & 0xff) * scale;
	int b = (color & 0xff) * scale;

	if(r > 255) r = 255;
	if(g > 255) g = 255;
	if(b > 255) b = 255;
	return (r << 16) | (g << 8) | b;
}

static int mix_color(int c1, float s1, int c2, float s2)
{
	int r = ((c1 >> 16) & 0xff) * s1 + ((c2 >> 16) & 0xff) * s2;
	int g = ((c1 >> 8) & 0xff) * s1 + ((c2 >> 8) & 0xff) * s2;
	int b = (c1 & 0xff) * s1 + (c2 & 0xff) * s2;

	if(r > 255) r = 255;
	if(g > 255) g = 255;
	if(b > 255) b = 255;
	return (r << 16) |(g << 8) | b;
}

BluebananaSlider::BluebananaSlider(BluebananaMain *plugin,
	BluebananaWindow *gui,
	int x,
	int y,
	int w,
	int h,
	float minval,
	float maxval)
 : BC_SubWindow(x, y, w, h)
{
	this->plugin = plugin;
	this->gui = gui;
	this->minval = minval;
	this->maxval = maxval;
	histval = NULL;
	histred = NULL;
	histgreen = NULL;
	histblue = NULL;
	drag = 0;
	light = -1;

	int bg_c = get_resources()->get_bg_color();
	int bg_r = ((bg_c >> 16) & 0xff);
	int bg_g = ((bg_c >> 8) & 0xff);
	int bg_b = (bg_c & 0xff);
	int bg_v = (bg_r * 76 + bg_g * 150 + bg_b * 29) >> 8;

	int dt_c = get_resources()->default_text_color;
	int dt_r = ((dt_c >> 16) & 0xff);
	int dt_g = ((dt_c >> 8) & 0xff);
	int dt_b = (dt_c & 0xff);
	int dt_v = (dt_r * 76 + dt_g * 150 + dt_b *29) >> 8;

	if(dt_v > bg_v)
	{
		// light text, dark background
		int dg_c = get_resources()->default_text_color;
		int dg_r = (((dg_c >> 16) & 0xff) + bg_r * 2) / 3;
		int dg_g = (((dg_c >> 8) & 0xff) + bg_g * 2) / 3;
		int dg_b = ((dg_c & 0xff) + bg_b * 2) / 3;
		dg_c = (dg_r << 16) | (dg_g << 8) | dg_b;

		active_bordercolor = get_resources()->default_text_color;
		inactive_bordercolor = dg_c;
		dimtextcolor = dg_c;
		troughcolor = scale_color(dg_c, .5);
		slidercolor = (bg_v > 0xa0 ? 0x606060 : 0xa0a0a0);
		sliderlit = (bg_v > 0xa0 ? 0x909070 : 0xd0d0b0);
		outline = 0x000000;
	}
	else
	{
		// dark text, light background
		active_bordercolor = scale_color(bg_c, 1.25);
		inactive_bordercolor = scale_color(bg_c, 1.01);
		troughcolor = scale_color(bg_c, .8);
		dimtextcolor = this->troughcolor;
		sliderlit =  scale_color(bg_c, 1.75);
		slidercolor = scale_color(this->sliderlit, .90);
		outline = 0x606060;
	}

	needlecolor = 0xe07000;
	needlelit = 0xff9000;
	highlight = 0;
	trough = NULL;
	troughlines = Z4 - Z1 - 1;
	troughcols = w - CW - CW - ZR - ZL;
}

BluebananaSlider::~BluebananaSlider()
{
	if(trough) delete trough;
	if(histval) delete[] histval;
	if(histred) delete[] histred;
	if(histgreen) delete[] histgreen;
	if(histblue) delete[] histblue;
}

void BluebananaSlider::reposition(int x, int y, int w, int h)
{
	BC_WindowBase::reposition_window(x, y, w, h);
	if(trough) delete trough;
	trough = NULL;
	if(histval) delete[] histval;
	histval = NULL;
	if(histred) delete[] histred;
	histred = NULL;
	if(histgreen) delete[] histgreen;
	histgreen = NULL;
	if(histblue) delete[] histblue;
	histblue = NULL;
	troughlines = Z4 - Z1 - 1;
	troughcols = w - CW - CW - ZR - ZL;
	update();
}

int BluebananaSlider::fraction_to_pixel(float input)
{
	return (int)rint((input * (troughcols - 1) + CW + ZL) + .001);
}

int BluebananaSlider::value_to_pixel(float input)
{
	return fraction_to_pixel((input - minval) / (maxval - minval));
}

float BluebananaSlider::pixel_to_fraction(int pixel)
{
	return (float)((pixel - CW - ZL) / (float)(troughcols - 1));
}

float BluebananaSlider::pixel_to_value(int pixel)
{
	float fraction = ((pixel - CW - ZL) / (float)(troughcols - 1));
	return fraction * (maxval - minval) + minval;
}

static int inside_box(int cx, int cy, int x, int y, int w, int h)
{
	return (cx >= x && cx < x + w && cy >= y && cy < y + h);
}

static int inside_vline(int cx, int cy, int x, int y0, int y1)
{
	if(cx == x)
	{
		if(cy >= y0 && cy <= y1) return 1;
		if(cy >= y1 && cy <= y0) return 1;
	}
	return 0;
}

static int inside_hline(int cx, int cy, int x0, int x1, int y)
{
	if(cy == y)
	{
		if(cx >= x0 && cx <= x1) return 1;
		if(cx >= x1 && cx <= x0) return 1;
	}
	return 0;
}

int BluebananaSlider::in_midslider(int x, int cx,int cy)
{
	int h = get_h();
	int y = (Z4 - Z1 + h) / 2;
	int r = Z1 - 2;
	if((cx - x) * (cx - x) + (cy - y) * (cy - y) < r * r ) return 1;
	return 0;
}

int BluebananaSlider::in_leftslider(int x,int cx, int cy)
{
	int i;
	int h = get_h();

	if(inside_hline(cx, cy, x - 1, x - Z1 * 2 - 1, Z4)) return 1;
	for(i = 1; i < Z1 + 1; i++)
	{
		if(inside_hline(cx, cy, x - (i * 2), x - Z1 * 2 - 1, Z4 - i)) return 1;
		if(inside_hline(cx, cy, x - (i * 2), x - Z1 * 2 - 1, Z4 + i)) return 1;
	}

	if(inside_vline(cx, cy, x - Z1 * 2 - 2, Z4 - Z1 + 1, Z4 + Z1 - 1)) return 1;
	if(inside_vline(cx, cy, x, Z1 + 1, h - 1)) return 1;

	return 0;
}

int BluebananaSlider::in_rightslider(int x, int cx, int cy)
{
	int i;
	int h = get_h();

	if(inside_hline(cx, cy, x + 1, x + Z1 * 2 + 1, Z4)) return 1;
	for(i = 1; i < Z1 + 1; i++)
	{
		if(inside_hline(cx, cy, x + (i * 2), x + Z1 * 2 + 1, Z4 - i)) return 1;
		if(inside_hline(cx, cy, x + (i * 2), x + Z1 * 2 + 1, Z4 + i)) return 1;
	}

	if(inside_vline(cx, cy, x + Z1 * 2 + 2, Z4 - Z1 + 1, Z4 + Z1 - 1))return 1;
	if(inside_vline(cx, cy, x, Z1 + 1, h - 1))return 1;
	return 0;
}

int BluebananaSlider::in_topslider(int x, int cx, int cy)
{
	int i;
	int h = get_h();

	if(inside_vline(cx, cy, x, Z1 * 2, 1)) return 1;
	for(i = 1; i < Z1 + 1; i++)
	{
		if(inside_vline(cx, cy, x + i, 1, Z1 * 2 - i * 2 + 1)) return 1;
		if(inside_vline(cx, cy, x - i, 1, Z1 * 2 - i * 2 + 1)) return 1;
	}
	if(inside_hline(cx, cy, x - Z1 + 1, x + Z1 - 1, 0)) return 1;
	return 0;
}

int BluebananaSlider::in_overslider(int x,int cx, int cy)
{
	int i;
	int h = get_h();
	int y0 = 0;
	int y1 = Z1 * 2 - 1;
	int w2 = CW; //Z1 * 2 - 1;

	if(cy <= Z1 && cx >= x - w2 && cx <= x + w2) return 1;
	return 0;
}

int BluebananaSlider::in_bottomslider(int x, int cx, int cy)
{
	int i;
	int h = get_h();

	if(inside_vline(cx, cy, x, h - Z1 * 2 - 1, h - 2))return 1;
	for(i = 1; i < Z1 + 1; i++)
	{
		if(inside_vline(cx, cy, x + i, h - 2, h - Z1 * 2 + i * 2 - 2)) return 1;
		if(inside_vline(cx, cy, x - i, h - 2, h - Z1 * 2 + i * 2 - 2)) return 1;
	}
	if(inside_hline(cx, cy, x - Z1 + 1, x + Z1 - 1, h - 1)) return 1;
	return 0;
}

int BluebananaSlider::in_leftslider2(int x, int cx)
{
	if(cx <= x + 2 && cx > x - CW) return 1;
	return 0;
}

int BluebananaSlider::in_rightslider2(int x, int cx)
{
	if(cx < x + CW && cx >= x - 2) return 1;
	return 0;
}

int BluebananaSlider::in_midslider2(int x, int cx)
{
	if(cx > x - CW / 2 && cx < x + CW / 2) return 1;
	return 0;
}

int BluebananaSlider::in_overslider2(int x,int cx, int cy)
{
	if(cx > x - CW / 2 && cx < x + CW / 2 && cy <= (Z1 + Z4) / 2) return 1;
	return 0;
}

void BluebananaSlider::draw_midslider(int x, int lit)
{
	int i;
	int h = get_h();
	int y0 = Z4 - Z1 + 2;
	int y1 = h - 2;

	set_color(lit ? sliderlit : slidercolor);
	draw_disc(x - Z1 + 2, y0, Z1 * 2 - 3, y1 - y0 + 1);

	set_color(outline);
	draw_circle(x - Z1 + 2, y0, Z1 * 2 - 3, y1 - y0 + 1);
}

void BluebananaSlider::draw_leftslider(int x, int lit)
{
	int i;
	int h = get_h();

	set_color(lit ? sliderlit : slidercolor);
	draw_line(x - 1, Z4, x - Z1 * 2 - 1, Z4);
	for(i=1; i < Z1 + 1; i++)
	{
		draw_line(x - (i * 2), Z4 - i, x - Z1 * 2 - 1, Z4 - i);
		draw_line(x - (i * 2), Z4 + i, x - Z1 * 2 - 1, Z4 + i);
	}

	set_color(outline);
	draw_line(x - 1, Z4, x - 1, Z4);
	draw_line(x - Z1 * 2 - 2, Z4 - Z1 + 1, x - Z1 * 2 - 2, Z4 + Z1 - 1);
	for(i = 1; i < Z1 + 1; i++)
	{
		draw_line(x - (i * 2), Z4 - i, x - (i * 2) - 1, Z4 - i);
		draw_line(x - (i * 2), Z4 + i, x - (i * 2) - 1, Z4 + i);
	}

	set_color(lit ? needlelit : needlecolor);
	draw_line(x, Z4 - Z1, x, Z4 + Z1);
}

void BluebananaSlider::draw_rightslider(int x, int lit)
{
	int i;
	int h = get_h();

	set_color(lit ? sliderlit : slidercolor);
	draw_line(x + 1, Z4, x + Z1 * 2 + 1, Z4);
	for(i = 1; i < Z1 + 1; i++)
	{
		draw_line(x + (i * 2), Z4 - i, x + Z1 * 2 + 1, Z4 - i);
		draw_line(x + (i * 2), Z4 + i, x + Z1 * 2 + 1, Z4 + i);
	}

	set_color(outline);
	draw_line(x + 1, Z4, x + 1, Z4);
	draw_line(x + Z1 * 2 + 2, Z4 - Z1 + 1, x + Z1 * 2 + 2, Z4 + Z1 - 1);
	for(i = 1; i < Z1 + 1; i++)
	{
		draw_line(x + (i * 2), Z4 - i, x + (i * 2) + 1, Z4 - i);
		draw_line(x + (i * 2), Z4 + i, x + (i * 2) + 1, Z4 + i);
	}

	set_color(lit ? needlelit : needlecolor);
	draw_line(x, Z4 - Z1, x, Z4 + Z1);
}

void BluebananaSlider::draw_topslider(int x, int lit)
{
	int i;
	int h = get_h();

	set_color(lit ? sliderlit : slidercolor);
	draw_line(x, Z1 * 2, x, 1);
	for(i = 1; i < Z1 + 1; i++)
	{
		draw_line(x + i, 1, x + i, Z1 * 2 - i * 2 + 1);
		draw_line(x - i, 1, x - i, Z1 * 2 - i * 2 + 1);
	}

	set_color(outline);
	draw_line(x - Z1 + 1, 0, x + Z1 - 1, 0);
	draw_line(x, Z1 * 2 + 1, x, Z1 * 2 + 1);
	for(i = 1; i < Z1 + 1; i++)
	{
		draw_line(x + i, Z1 * 2 - i * 2 + 1, x + i, Z1 * 2 - i * 2 + 2);
		draw_line(x - i, Z1 * 2 - i * 2 + 1, x - i, Z1 * 2 - i * 2 + 2);
	}
}

void BluebananaSlider::draw_bottomslider(int x, int lit)
{
	int i;
	int h = get_h();

	set_color(lit ? sliderlit : slidercolor);
	draw_line(x, h - Z1 * 2 - 1, x, h - 2);
	for(i = 1; i < Z1 + 1; i++)
	{
		draw_line(x + i, h - 2, x + i, h - Z1 * 2 + i * 2 - 2);
		draw_line(x - i, h - 2, x - i, h - Z1 * 2 + i * 2 - 2);
	}

	set_color(outline);
	draw_line(x - Z1 + 1, h - 1, x + Z1 - 1, h - 1);
	draw_line(x, h - Z1 * 2 - 2, x, h - Z1 * 2 - 2);
	for(i = 1; i < Z1 + 1; i++)
	{
		draw_line(x + i, h - Z1 * 2 + i * 2 - 2, x + i, h - Z1 * 2 + i * 2 - 3);
		draw_line(x - i, h - Z1 * 2 + i * 2 - 2, x - i, h - Z1 * 2 + i * 2 - 3);
	}
}

void BluebananaSlider::draw_overslider(int x, int lit)
{
	int i;
	int h = get_h();
	int y0 = 0;
	int y1 = Z1 * 2 - 1;
	int w2 = CW + 1;

	int bg_c = get_resources()->get_bg_color();
	int bg_r = ((bg_c >> 16) & 0xff);
	int bg_g = ((bg_c >> 8) & 0xff);
	int bg_b = (bg_c & 0xff);
	float del = lit ? 1. : .9;
	set_color(((int)(bg_r * del) << 16) | ((int)(bg_g * del) << 8) | ((int)(bg_b * del)));
	draw_box(x - w2 + 1, 0, w2 * 2 - 2, Z1);
	draw_box(x - w2, 1, w2 * 2 + 1, Z1 - 2);

	set_color(lit ? sliderlit : slidercolor);
	draw_box(x - w2 + 2, 1, w2 * 2 - 3, Z1 - 2);
	draw_box(x - w2 + 1, 2, w2 * 2 - 1, Z1 - 4);

	set_color(((int)(bg_r * del) << 16) | ((int)(bg_g * del) << 8) | ((int)(bg_b * del)));
	draw_line(x, 2, x, Z1 - 3);
}

int BluebananaSlider::button_release_event()
{
	if(is_event_win())
	{
		if(drag > 0)
		{
			drag = 0;
			update();
			cursor_motion_event();
		}
		return 1;
	}
	return 0;
}

void BluebananaSlider::update()
{
	int w = get_w();
	int h = get_h();
	int bg_c = get_resources()->get_bg_color();
	int bg_r = ((bg_c >> 16) & 0xff);
	int bg_g = ((bg_c >> 8) & 0xff);
	int bg_b = (bg_c & 0xff);
	int b_c = highlight ? active_bordercolor : inactive_bordercolor;
	int b_r = ((b_c >> 16) & 0xff);
	int b_g = ((b_c >> 8) & 0xff);
	int b_b = (b_c & 0xff);
	int tb_c = troughcolor;
	int tb_r = ((tb_c >> 16) & 0xff);
	int tb_g = ((tb_c >> 8) & 0xff);
	int tb_b = (tb_c & 0xff);

	int ZH = Z4 - Z1 + 1;
	int ZHC = Z4 - ZC + 1;

	int i, j;

	clear_box(0, 0, w, h);

	// trough shadow
	float del = .9;
	int shadow_c = ((int)(bg_r * del) << 16) | ((int)(bg_g * del) << 8) | ((int)(bg_b * del));
	set_color(shadow_c);
	draw_box(0, Z1, w, ZH + 2);
	draw_box(1, Z1 - 1, w - 2, ZH + 4);

	del = .8;
	shadow_c = ((int)(bg_r * del) << 16) | ((int)(bg_g * del) << 8) | ((int)(bg_b * del));
	set_color(shadow_c);
	draw_box(1, Z1 + 1, w - 2, ZH);
	draw_box(2, Z1, w - 4, ZH + 2);

	del = .7;
	shadow_c = ((int)(bg_r * del) << 16) | ((int)(bg_g * del) << 8) | ((int)(bg_b * del));
	set_color(shadow_c);
	draw_box(2, Z1 + 2, w - 4, ZH - 2);
	draw_box(3, Z1 + 1, w - 6, ZH);

	// trough border
	set_color(b_c);
	draw_box(1, Z1 + 1, w - 4, ZH - 2);
	draw_box(2 , Z1, w - 6, ZH);

	// trough inner
	int ntw = troughcols;
	int nth = troughlines;
	int ntx = CW + ZL;
	int nty = Z1 + 1;

	if(!trough)
	{
		trough = new VFrame(NULL, ntw, nth);
	}

	if(!trough)
	{
		fprintf(stderr, "Bluebanana: Unable to create Frame for slider\n");
	}
	else
	{
		unsigned char *data = trough->get_data();
		long bpr = trough->get_bytes_per_line();
		float ha = 1.;
		if(!highlight)
		{
			ha *= .25;
			tb_r = bg_r;
			tb_g = bg_g;
			tb_b = bg_b;
		}

		if(histval)  // trough is a histogram and has data
		{
			long bpp = trough->get_bytes_per_pixel();
			float rr, gg, bb, aa;

			for(i = 0; i < ntw; i++)
			{
				unsigned char *row = data;
				float y = nth - histval[i];
				float y0 = i > 0 ? nth - histval[i - 1] : y;
				float y1 = i + 1 < ntw ? nth - histval[i + 1] : y;
				float yp = MIN(y, MIN(y0 + 1, y1 + 1));
				float y3 = (y0 + y + y1) * .333f;
				float r, g, b, a;

				trough_color((i + .5f) / ntw, 0.f, rr, gg, bb, aa);
				rr *= 255;
				gg *= 255;
				bb *= 255;
				rr = CLAMP((rr + b_r * 3) / 4, 0, 255);
				gg = CLAMP((gg + b_g * 3) / 4, 0, 255);
				bb = CLAMP((bb + b_b * 3) / 4, 0, 255);

				if(histred)
				{
					r = histred[i];
					g = histgreen[i];
					b = histblue[i];
				}
				else
				{
					trough_color((i + .5) / ntw, 0, r, g, b, a);
					r = CLAMP(r, 0, 255);
					g = CLAMP(g, 0, 255);
					b = CLAMP(b, 0, 255);
				}

				for(j = 0; j < yp - 1 && j < nth; j++)
				{
					row[0] = tb_r;
					row[1] = tb_g;
					row[2] = tb_b;
					row[3] = 0xff;
					row += bpr;
				}

				a = .75 * ha;
				for(; j < y && j < nth; j++)
				{
					row[0] = rint(rr * a + tb_r * (1 - a));
					row[1] = rint(gg * a + tb_g * (1 - a));
					row[2] = rint(bb * a + tb_b * (1 - a));
					row[3] = 0xff;
					row += bpr;
				}

				a = ha;
				for(; j < nth; j++)
				{
					float a = CLAMP(1. - .3 * (float)(j - y3) / (nth - y3 + 1), 0.f, 1.f) * ha;
					row[0] = rint(r * a * 255.f + tb_r * (1 - a));
					row[1] = rint(g * a * 255.f + tb_g * (1 - a));
					row[2] = rint(b * a * 255.f + tb_b * (1 - a));
					row[3] = 0xff;
					row += bpr;
				}

				data += bpp;
			}
		}
		else
		{
			for(i = 0; i < ntw; i++)
			{
				unsigned char *row = data + i * trough->get_bytes_per_pixel();
				for(j = 0; j < nth; j++)
				{
					float r, g, b, a;
					trough_color((i + .5f) / ntw, (j + .5) / nth, r, g, b, a);
					r = CLAMP(r, 0, 255);
					g = CLAMP(g, 0, 255);
					b = CLAMP(b, 0, 255);

					a *= ha;
					row[0] = rint(r * a * 255 + tb_r * (1 - a));
					row[1] = rint(g * a * 255 + tb_g * (1 - a));
					row[2] = rint(b * a * 255 + tb_b * (1 - a));
					row[3] = 0xff;
					row += bpr;
				}
			}
		}
		draw_vframe(trough, ntx, nty);
	}
}

// quick/dirty unnormalized linear resample from native -> display size
static void resample_histogram(float *raw, float *raw_red, float *raw_green, float *raw_blue,
	float *out, float *out_red, float *out_green, float *out_blue,
	int raw_n, int raw_off, int out_n, int out_start, int out_end)
{
	float coverage = (float)raw_n / out_n;
	float scale = coverage;
	if(coverage < 1.) coverage = 1.;
	float icoverage = 1. / coverage;

	float raw_center = out_start * scale + raw_off;
	float raw_start = raw_center - coverage;
	float raw_end = raw_center + coverage;

	for(int i = out_start; i < out_end; i++)
	{
		int j = raw_start + 1;
		int e = raw_end;
		if(j < 0) j = 0;
		if(e > HISTSIZE) e = HISTSIZE;
		int jj = j;
		float k = (j - raw_start) * icoverage - 1.f;
		float a = 0;

		for(; j <= e; j++)
		{
			float ik = 1. - fabs(k);
			a += raw[j] * ik;
			k += icoverage;
		}
		out[i] = a;
		raw_start += scale;
		raw_end += scale;
	}

	if(raw_red)
	{
		coverage = (float)raw_n / out_n;
		scale = coverage;
		if(coverage < (1 << HRGBSHIFT)) coverage = (1 << HRGBSHIFT);
		icoverage = 1. / coverage;

		raw_center = out_start * scale + raw_off;
		raw_start = raw_center - coverage;
		raw_end = raw_center + coverage;

		for(int i = out_start; i < out_end; i++)
		{
			int j = raw_start + 1;
			int e = raw_end;
			if(j < 0) j = 0;
			if(e > HISTSIZE) e = HISTSIZE;
			int jj = j;
			float k = (j - raw_start) * icoverage - 1.f;
			float r = 0.f, g = 0.f, b = 0.f, a = 0.f;
			for(; j <= e; j++)
			{
				float ik = 1. - fabs(k);
				int sj = (j >> HRGBSHIFT);
				int sjs = sj << HRGBSHIFT;

#if (HRGBSHIFT == 0)
				a += raw[sjs] * ik;
#endif
#if (HRGBSHIFT == 1)
				a += (raw[sjs] +
					raw[sjs + 1]) * ik;
#endif
#if (HRGBSHIFT == 2)
				a += (raw[sjs] +
					raw[sjs + 1] +
					raw[sjs + 2] +
					raw[sjs + 3]) * ik;
#endif
#if (HRGBSHIFT == 3)
				a += (raw[sjs] +
					raw[sjs + 1] +
					raw[sjs + 2] +
					raw[sjs + 3] +
					raw[sjs + 4] +
					raw[sjs + 5] +
					raw[sjs + 6] +
					raw[sjs + 7]) * ik;
#endif
				r += raw_red[sj] * ik;
				g += raw_green[sj] * ik;
				b += raw_blue[sj] * ik;
				k += icoverage;
			}

			a = a ? 1.f / a : 0;
			out_red[i] = CLAMP(r * a, 0.f, 1.f);
			out_green[i] = CLAMP(g * a, 0.f, 1.f);
			out_blue[i] = CLAMP(b * a, 0.f, 1.f);
			raw_start += scale;
			raw_end += scale;
		}
	}
}

void BluebananaSlider::update_histogram(float *raw, float *red, float *green, float *blue, int n)
{
	int i, j;
	int h = troughlines;
	int w = troughcols;
	float il2n = 1. / log2(n);

	if(!histval)
		histval = new float[w];

	if(!histred && red)
	{
		histred = new float[w];
		histgreen = new float[w];
		histblue = new float[w];
	}

	// resample center
	resample_histogram(raw, red, green, blue, histval, histred, histgreen, histblue,
		HISTSIZE, 0, w, 0, w);

	// select value normalization
	for(i = 0; i < w; i++)
	{
		if(histval[i] <= .0001f)
		{
			histval[i] = -2;
		}
		else
		{
			float val = log2(histval[i]) * il2n;
			val = pow(val < 0 ? 0 : val, .75) * h;
			if(val < 0) val = 0;
			if(val > h) val = h;
			histval[i] = val;
		}
	}
	render();
}

// ---------------------------- Single ----------------------------------

BluebananaSliderSingle::BluebananaSliderSingle(BluebananaMain *plugin,
	BluebananaWindow *gui,
	int x,
	int y,
	int w,
	int h,
	float minval,
	float maxval)
 : BluebananaSlider(plugin,gui,x,y,w,h,minval,maxval)
{
	val = (minval + maxval) / 2;
	increment = 1;
}

int BluebananaSliderSingle::button_press_event()
{
	if(is_event_win() && cursor_inside())
	{
		int min;
		int max;
		int cx = get_cursor_x();

		gui->deactivate();

		if(get_buttonpress() == 4)
		{
			set_val(val - increment);
			return 1;
		}
		else if(get_buttonpress() == 5)
		{
			set_val(val + increment);
			return 1;
		}

		switch(light)
		{
		case 0:
			xoff = cx - value_to_pixel(val);
			break;
		default:
			return 1;
		}

		drag = 1;
		update();
		return 1;
	}
	return 0;
}

int BluebananaSliderSingle::cursor_motion_event()
{
	int cx = get_cursor_x();
	int cy = get_cursor_y();

	if(drag)
	{
		switch(light)
		{
		case 0:
			set_val(pixel_to_value(cx - xoff));
			break;
		default:
			drag = 0;
			light = -1;
			update();
			break;
		}
		return 1;
	}
	else
	{
		if(is_event_win() && cursor_inside())
		{
			if(in_midslider2(value_to_pixel(val), cx))
			{
				if(light != 0)
				{
					light = 0;
					update();
				}
			}
			else
			{
				if(light != -1)
				{
					light = -1;
					update();
				}
			}
			return 1;
		}
		else
		{
			if(light != -1)
			{
				light = -1;
				update();
			}
		}
	}
	return 0;
}

void BluebananaSliderSingle::set_val(float v)
{
	val = v;
	if(val < minval) val = minval;
	if(val > maxval) val = maxval;
	handle_event();
	update();
}

void BluebananaSliderSingle::update()
{
	BluebananaSlider::update();
	draw_bottomslider(value_to_pixel(val), light == 0);
	flash();
	flush();
}

// ---------------------------- Bracket ----------------------------------
BluebananaSliderBracket::BluebananaSliderBracket(BluebananaMain *plugin,
	BluebananaWindow *gui,
	int x,
	int y,
	int w,
	int h,
	float minval,
	float maxval)
 : BluebananaSlider(plugin,gui,x,y,w,h,minval,maxval)
{
	loval = minval;
	midval = (minval + maxval) / 2;
	hival = maxval;
	overval = minval;
	increment = 1;
}

int BluebananaSliderBracket::button_press_event()
{
	if(is_event_win() && cursor_inside())
	{
		int min;
		int max;
		int cx = get_cursor_x();

		gui->deactivate();

		if(shift_down())
		{
			// narrow/widen range
			if(get_buttonpress() == 4)
			{
				set_delta(increment * 2);
				return 1;
			}
			else if(get_buttonpress() == 5)
			{
				set_delta(-increment * 2);
				return 1;
			}
		}
		else if (ctrl_down())
		{
			// shift all vals up/down
			if(get_buttonpress() == 4)
			{
				set_mid(midval - increment);
				return 1;
			}
			else if(get_buttonpress() == 5)
			{
				set_mid(midval + increment);
				return 1;
			}
		}
		else
		{
			// increase/decrease overlap
			if(get_buttonpress() == 4)
			{
				set_over(overval - 1);
				return 1;
			}
			else if(get_buttonpress() == 5)
			{
				set_over(overval + 1);
				return 1;
			}
		}

		switch(light)
		{
		case 0:
			xoff = cx - value_to_pixel(loval);
			break;
		case 1:
			xoff = cx - value_to_pixel(midval);
			break;
		case 2:
			xoff = cx - value_to_pixel(hival);
			break;
		case 3:
			xoff = cx - value_to_pixel(overval);
			break;
		default:
			return 1;
		}

		drag = 1;
		update();
		return 1;
	}
	return 0;
}

int BluebananaSliderBracket::cursor_motion_event()
{
	int cx = get_cursor_x();
	int cy = get_cursor_y();

	if(drag)
	{
		switch(light)
		{
		case 0:
			set_lo(pixel_to_value(cx - xoff));
			break;
		case 1:
			set_mid(pixel_to_value(cx - xoff));
			break;
		case 2:
			set_hi(pixel_to_value(cx - xoff));
			break;
		case 3:
			set_over(pixel_to_value(cx - xoff));
			break;
		default:
			drag = 0;
			light = -1;
			update();
			break;
		}
		return 1;
	}
	else
	{
		if(is_event_win() && cursor_inside())
		{
			if(in_overslider(value_to_pixel(overval), cx, cy))
			{
				if(light != 3)
				{
					light = 3;
					update();
				}
			}
			else if(in_midslider(value_to_pixel(midval), cx, cy))
			{
				if(light != 1)
				{
					light = 1;
					update();
				}
			}
			else if(in_leftslider(value_to_pixel(loval), cx, cy))
			{
				if(light != 0)
				{
					light = 0;
					update();
				}
			}
			else if(in_rightslider(value_to_pixel(hival), cx, cy))
			{
				if(light != 2)
				{
					light = 2;
					update();
				}
			}
			else if(in_midslider2(value_to_pixel(midval), cx))
			{
				if(light != 1)
				{
					light = 1;
					update();
				}
			}
			else if(in_rightslider2(value_to_pixel(hival), cx))
			{
				if(light != 2)
				{
					light = 2;
					update();
				}
			}
			else if(in_leftslider2(value_to_pixel(loval), cx))
			{
				if(light != 0)
				{
					light = 0;
					update();
				}
			}
			else if(in_overslider2(value_to_pixel(overval), cx, cy))
			{
				if(light != 3)
				{
					light = 3;
					update();
				}
			}
			else
			{
				if(light != -1)
				{
					light = -1;
					update();
				}
			}
			return 1;
		}
		else
		{
			if(light != -1)
			{
				light = -1;
				update();
			}
		}
	}
	return 0;
}

void BluebananaSliderBracket::set_lo(float val)
{
	if(val < minval) val = minval;
	if(val > maxval) val = maxval;
	if(val > hival) val = hival;
	loval = val;

	if(hival < loval) hival += (maxval - minval);
	if(midval < loval) midval += (maxval - minval);
	midval = (hival + loval) / 2.;
	if(hival > maxval) hival -= (maxval - minval);
	if(midval > maxval) midval -= (maxval - minval);
	if(loval > maxval) loval -= (maxval - minval);
	if(hival < minval) hival += (maxval - minval);
	if(midval < minval) midval += (maxval - minval);
	if(loval < minval) loval += (maxval - minval);
	handle_event();
	update();
}

void BluebananaSliderBracket::set_mid(float val)
{
	float oldmid = midval;
	if(val < minval) val = minval;
	if(val > maxval) val = maxval;
	midval = val;

	float delta = (hival + loval) / 2.;
	hival += midval - delta;
	loval += midval - delta;
	if(hival > maxval)
	{
		loval += maxval - hival;
		midval += maxval - hival;
		hival = maxval;
	}
	if(loval < minval)
	{
		hival -= loval;
		midval -= loval;
		loval = minval;
	}
	handle_event();
	update();
}

void BluebananaSliderBracket::set_hi(float val)
{
	if(val < minval) val = minval;
	if(val > maxval) val = maxval;
	if(val < loval) val = loval;
	hival = val;

	if(loval > hival) loval -= (maxval - minval);
	if(midval > hival) midval -= (maxval - minval);
	midval = (hival + loval) / 2.;
	if(hival > maxval) hival -= (maxval - minval);
	if(midval > maxval) midval -= (maxval - minval);
	if(loval > maxval) loval -= (maxval - minval);
	if(hival < minval) hival += (maxval - minval);
	if(midval < minval) midval += (maxval - minval);
	if(loval < minval) loval += (maxval - minval);

	handle_event();
	update();
}

void BluebananaSliderBracket::set_delta(float incr)
{
	float delta = (hival-loval) + incr;
	if(delta < minval) delta = minval;
	if(delta > maxval) delta = maxval;
	loval = midval - delta / 2;
	hival = loval + delta;

	if(loval < minval)
	{
		hival -= loval;
		midval -= loval;
		loval = minval;
	}
	if(hival > maxval)
	{
		loval -= (hival - maxval);
		midval -= (hival - maxval);
		hival = maxval;
	}
	handle_event();
	update();
}

void BluebananaSliderBracket::set_over(float val)
{
	if(val < minval) val = minval;
	if(val > maxval) val = maxval;
	overval = val;
	handle_event();
	update();
}

void BluebananaSliderBracket::update()
{
	BluebananaSlider::update();
	draw_overslider(value_to_pixel(overval), light == 3);
	draw_leftslider(value_to_pixel(loval), light == 0);
	draw_rightslider(value_to_pixel(hival), light == 2);
	draw_midslider(value_to_pixel(midval), light == 1);
	flash();
	flush();
}

// ----------------------- Circular Bracket -----------------------------
BluebananaSliderCircular::BluebananaSliderCircular(BluebananaMain *plugin,
	BluebananaWindow *gui,
	int x,
	int y,
	int w,
	int h,
	float minval,
	float maxval)
 : BluebananaSliderBracket(plugin, gui, x, y, w, h, minval, maxval)
{
}

void BluebananaSliderCircular::set_lo(float val)
{
	if(val < minval) val = minval;
	if(val > maxval) val = maxval;
	loval = val;

	if(hival < loval) hival += (maxval - minval);
	if(midval < loval) midval += (maxval - minval);
	midval = (hival + loval) / 2.;
	if(hival > maxval) hival -= (maxval - minval);
	if(midval > maxval) midval -= (maxval - minval);
	if(loval > maxval) loval -= (maxval - minval);
	if(hival < minval) hival += (maxval - minval);
	if(midval < minval) midval += (maxval - minval);
	if(loval < minval) loval += (maxval - minval);

	handle_event();
	update();
}

void BluebananaSliderCircular::set_mid(float val)
{
	float oldmid = midval;
	if(val < minval) val = minval;
	if(val > maxval) val = maxval;
	midval = val;
	if(hival < oldmid) hival += (maxval - minval);
	if(loval > oldmid) loval -= (maxval - minval);
	float delta = hival - loval;
	loval = midval - delta / 2;
	hival = loval + delta;
	if(hival > maxval) hival -= (maxval - minval);
	if(loval > maxval) loval -= (maxval - minval);
	if(hival < minval) hival += (maxval - minval);
	if(loval < minval) loval += (maxval - minval);
	handle_event();
	update();
}

void BluebananaSliderCircular::set_hi(float val)
{
	if(val < minval) val = minval;
	if(val > maxval) val = maxval;
	hival = val;
	if(loval > hival) loval -= (maxval - minval);
	if(midval > hival) midval -= (maxval - minval);
	midval = (hival + loval) / 2.;
	if(hival > maxval) hival -= (maxval - minval);
	if(midval > maxval) midval -= (maxval - minval);
	if(loval > maxval) loval -= (maxval - minval);
	if(hival < minval) hival += (maxval - minval);
	if(midval < minval) midval += (maxval - minval);
	if(loval < minval) loval += (maxval - minval);
	handle_event();
	update();
}

void BluebananaSliderCircular::set_delta(float incr)
{
	if(hival < midval) hival += (maxval - minval);
	if(loval > midval) loval -= (maxval - minval);

	float delta = (hival - loval) + incr;
	if(delta < minval) delta = minval;
	if(delta > maxval) delta = maxval;
	loval = midval - delta / 2;
	hival = loval + delta;

	if(hival > maxval) hival -= (maxval - minval);
	if(loval > maxval) loval -= (maxval - minval);
	if(hival < minval) hival += (maxval - minval);
	if(loval < minval) loval += (maxval - minval);
	handle_event();
	update();
}

void BluebananaSliderCircular::update()
{
	BluebananaSliderBracket::update();
	flash();
	flush();
}

// -------------------------- In + Out + Gamma --------------------------------

#define mG2 (2.f - 2.f * MIN_GAMMA)
#define MG2 (2.f * MAX_GAMMA - 2.f)

float BluebananaSliderChannel::pixel_to_gamma(float pixel)
{
	float unit = (1. - pixel_to_fraction(pixel) - FRAC0) / (FRAC100 - FRAC0);
	return (unit > .5f ?
			(1.f - unit) * mG2 + MIN_GAMMA :
			(.5f - unit) * MG2 + 1.f );
}

float BluebananaSliderChannel::gamma_to_pixel(float gamma)
{
	float fraction =
		gamma < 1 ? 1.f - (gamma - MIN_GAMMA) / mG2 :
		.5f - (gamma - 1.f) / MG2 ;
	return fraction_to_pixel((1. - fraction) * (FRAC100 - FRAC0) + FRAC0);
}

float BluebananaSliderChannel::pixel_to_value(float pixel)
{
	if(pixel < t_x1)
	{
		return (t_x0 - pixel) / t_w01 * MIN_HISTBRACKET + MIN_HISTBRACKET;
	}
	else if(pixel < t_x2)
	{
		return (pixel - t_x1) / t_w12 * 100;
	}
	else
	{
		return (pixel - t_x2) / t_w23 * (MAX_HISTBRACKET - 100) + 100;
	}
}

float BluebananaSliderChannel::value_to_pixel(float val)
{
	if(val < 0)
	{
		return rint(MIN_HISTBRACKET - val) / MIN_HISTBRACKET * t_w01 + t_x0;
	}
	else if(val < 100)
	{
		return rint(val / 100. * t_w12 + t_x1 + .01);
	}
	else
	{
		return rint((val - 100) / (MAX_HISTBRACKET - 100.) * t_w23 + t_x2);
	}
}

BluebananaSliderChannel::BluebananaSliderChannel(BluebananaMain *plugin,
		BluebananaWindow *gui,
		int x,
		int y,
		int w,
		int h)
	: BluebananaSlider(plugin, gui, x, y, w, h, 0, 100)
{
	loval = 0;
	gamma = 1.;
	hival = 100.;
}

int BluebananaSliderChannel::button_press_event()
{
	if(is_event_win() && cursor_inside())
	{
		int min;
		int max;
		int cx = get_cursor_x();
		int cy = get_cursor_y();

		gui->deactivate();

		if(shift_down())
		{
			// narrow/widen range
			if(get_buttonpress() == 4)
			{
				set_range(hival - loval - 2);
				return 1;
			}
			else if(get_buttonpress() == 5)
			{
				if(loval < hival)
				{
					set_range(hival - loval + 2);
				}
				return 1;
			}
		}
		else if (ctrl_down())
		{
			// shift all vals up/down
			if(get_buttonpress() == 4)
			{
				set_mid((loval + hival) / 2 + 1);
				return 1;
			}
			else if(get_buttonpress() == 5)
			{
				set_mid((loval + hival) / 2 - 1);
				return 1;
			}
		}
		else
		{
			// increase/decrease gamma 
			if(get_buttonpress() == 4)
			{
				set_gamma(gamma - .01);
				return 1;
			}
			else if(get_buttonpress() == 5)
			{
				set_gamma(gamma + .01);
				return 1;
			}
		}

		switch(light)
		{
		case 0:
			xoff = cx - value_to_pixel(loval);
			break;
		case 1:
			xoff = cx - value_to_pixel((loval + hival)/2);
			break;
		case 2:
			xoff = cx - value_to_pixel(hival);
			break;
		case 3:
			xoff = cx - gamma_to_pixel(gamma);
			break;
		default:
			return 1;
		}

		drag = 1;
		update();
		return 1;
	}
	return 0;
}

int BluebananaSliderChannel::cursor_motion_event()
{
	int cx = get_cursor_x();
	int cy = get_cursor_y();

	if(drag)
	{
		switch(light)
		{
		case 0:
			set_lo(pixel_to_value(cx - xoff));
			break;
		case 1:
			set_mid(pixel_to_value(cx - xoff));
			break;
		case 2:
			set_hi(pixel_to_value(cx - xoff));
			break;
		case 3:
			set_gamma(pixel_to_gamma(cx - xoff));
			break;
		default:
			drag = 0;
			light = -1;
			update();
			break;
		}
		return 1;
	}
	else
	{
		if(is_event_win() && cursor_inside())
		{
			if(in_topslider(gamma_to_pixel(gamma), cx, cy))
			{
				if(light != 3)
				{
					light = 3;
					update();
				}
			}
			else if(in_midslider(value_to_pixel((loval + hival) / 2), cx, cy))
			{
				if(light != 1)
				{
					light = 1;
					update();
				}
			}
			else if(in_leftslider(value_to_pixel(loval), cx, cy))
			{
				if(light != 0)
				{
					light = 0;
					update();
				}
			}
			else if(in_rightslider(value_to_pixel(hival), cx, cy))
			{
				if(light != 2)
				{
					light = 2;
					update();
				}
			}
			else if(in_bottomslider(value_to_pixel(hival), cx, cy))
			{
				if(light != 2)
				{
					light = 2;
					update();
				}
			}
			else if(in_bottomslider(value_to_pixel(loval), cx, cy))
			{
				if(light != 0)
				{
					light = 0;
					update();
				}
			}
			else if(in_midslider2(value_to_pixel((loval + hival) / 2), cx))
			{
				if(light != 1)
				{
					light = 1;
					update();
				}
			}
			else if(in_overslider2(gamma_to_pixel(gamma), cx, cy))
			{
				if(light != 3)
				{
					light = 3;
					update();
				}
			}
			else
			{
				if(light != -1)
				{
					light = -1;
					update();
				}
			}
			return 1;
		}
		else
		{
			if(light != -1)
			{
				light = -1;
				update();
			}
		}
	}
	return 0;
}

void BluebananaSliderChannel::set_lo(float v)
{
	if(v > hival) v = hival;
	if(v < -100) v = -100;
	if(v > 100) v = 100;
	loval = rint(v);
	handle_event();
	update();
}

void BluebananaSliderChannel::set_hi(float v)
{
	if(v < loval) v =loval;
	if(v < 0) v = 0;
	if(v > 200) v = 200;
	hival = rint(v);
	handle_event();
	update();
}

void BluebananaSliderChannel::set_mid(float v)
{
	float mid = (hival + loval) * .5;
	float range = hival - loval;
	float del = v - mid;

	if(hival + del > 200) del = 200 - hival;
	if(hival + del < 0) del = 0 - hival;
	if(loval + del > 100) del = 100 - loval;
	if(loval + del < -100) del= -100 - loval;

	loval += rint(del);
	hival = loval + rint(range);
	handle_event();
	update();
}

void BluebananaSliderChannel::set_range(float v)
{
	float mid = (hival + loval) * .5;
	float range = hival - loval;

	if(v > 200) v = 200;
	if(v < 0) v = 0;

	float del = (v - range) / 2;

	if(hival + del > 200) del = 200 - hival;
	if(hival + del < 0) del = 0 - hival;

	if(loval - del > 100) del = loval - 100;
	if(loval - del < -100) del = loval + 100;

	loval -= rint(del);
	hival = rint(loval + v);
	handle_event();
	update();
}

void BluebananaSliderChannel::set_gamma(float v)
{
	if(v > MAX_GAMMA) v = MAX_GAMMA;
	if(v < MIN_GAMMA) v = MIN_GAMMA;
	gamma = v;
	handle_event();
	update();
}

void BluebananaSliderChannel::update()
{
	int w = get_w();
	int h = get_h();
	int bg_c = get_resources()->get_bg_color();
	int bg_r = ((bg_c >> 16) & 0xff);
	int bg_g = ((bg_c >> 8) & 0xff);
	int bg_b = (bg_c & 0xff);
	int b_c = highlight ? active_bordercolor : inactive_bordercolor;
	int b_r = ((b_c >> 16) & 0xff);
	int b_g = ((b_c >> 8) & 0xff);
	int b_b = (b_c & 0xff);
	int tb_c = troughcolor;
	int tb_r = ((tb_c >> 16) & 0xff);
	int tb_g = ((tb_c >> 8) & 0xff);
	int tb_b = (tb_c & 0xff);

	int ZH = Z4 - Z1 + 1;
	int ZHC = Z4 - ZC + 1;

	int i, j;

	t_x0 = fraction_to_pixel(0);
	t_x1 = fraction_to_pixel(FRAC0);
	t_x2 = fraction_to_pixel(FRAC100);
	t_x3 = fraction_to_pixel(1.);
	t_w01 = t_x1 - t_x0;
	t_w12 = t_x2 - t_x1;
	t_w23 = t_x3 - t_x2;


	int ntw = troughcols - 6;
	int nth = troughlines;
	int ntx = t_x0;
	int nty = Z1 + 1;

	clear_box(0, 0, w, h);

	// trough shadow

	// w+4 h+4 x-1 y-1
	float del = .9;
	int shadow_c = ((int)(bg_r * del) << 16) | ((int)(bg_g * del) << 8) | ((int)(bg_b * del));
	set_color(shadow_c);
	draw_box(0, Z1, t_x1 +3, ZH + 2);
	draw_box(1,Z1 - 1, t_x1 + 1, ZH + 4);
	draw_box(t_x1, Z1, t_w12 + 3, ZH + 2);
	draw_box(t_x1 + 1, Z1 - 1, t_w12 + 1, ZH + 4);
	draw_box(t_x2, Z1, w - t_x2, ZH - 2 + 2);
	draw_box(t_x2 + 1, Z1 - 1, w - t_x2 - 2, ZH + 4);

	// w+2 h+2 x y
	del = .8;
	shadow_c = ((int)(bg_r*del) << 16) | ((int)(bg_g*del) << 8) | ((int)(bg_b*del));
	set_color(shadow_c);
	draw_box(1, Z1 + 1, t_x1 + 1, ZH);
	draw_box(2, Z1 , t_x1 - 1, ZH + 2);
	draw_box(t_x1 + 1, Z1 + 1, t_w12 + 1, ZH);
	draw_box(t_x1 + 2, Z1, t_w12 - 1, ZH + 2);
	draw_box(t_x2 + 1,Z1 + 1, w - t_x2 - 2, ZH);
	draw_box(t_x2 + 2, Z1 , w - t_x2 - 4, ZH + 2);

	// w h x+1 y+1
	del = .7;
	shadow_c = ((int)(bg_r * del) << 16) | ((int)(bg_g * del) << 8) | ((int)(bg_b * del));
	set_color(shadow_c);
	draw_box(2, Z1 + 2, t_x1 - 1, ZH - 2);
	draw_box(3, Z1 + 1, t_x1 - 3, ZH);
	draw_box(t_x1 + 2, Z1 + 2, t_w12 - 1, ZH - 2);
	draw_box(t_x1 + 3, Z1 + 1, t_w12 - 3, ZH);
	draw_box(t_x2 + 2, Z1 + 2, w - t_x2 - 4, ZH - 2);
	draw_box(t_x2 + 3, Z1 + 1, w - t_x2 - 6, ZH);

	// trough border
	set_color(b_c);
	draw_box(1, Z1 + 1, t_x1 - 1,ZH - 2);
	draw_box(2, Z1, t_x1 - 3, ZH);
	draw_box(t_x1 + 1, Z1 + 1, t_w12 - 1, ZH - 2);
	draw_box(t_x1 + 2, Z1, t_w12 - 3, ZH);
	draw_box(t_x2 + 1, Z1 + 1, w - t_x2 - 4, ZH - 2);
	draw_box(t_x2+2, Z1, w - t_x2 - 6, ZH);

	// trough inner
	// the trough in the three-part hist slider is slightly smaller than
	//  the full slider range, and cut into three sections
	if(!trough)
	{
		trough = new VFrame(NULL, ntw, nth);
	}

	if(!trough)
	{
		fprintf(stderr, "Bluebanana: Unable to create Frame for slider\n");
	}
	else
	{
		unsigned char *data = trough->get_data();
		float ha = 1.;
		long bpp = trough->get_bytes_per_pixel();
		long bpr = trough->get_bytes_per_line();
		float r, g, b, a, rr, gg, bb;

		if(!highlight)
		{
			ha *= .25;
			tb_r = bg_r;
			tb_g = bg_g;
			tb_b = bg_b;
		}

		if(!histval)
		{

			for(i = 0; i < ntw; i++)
			{
				unsigned char *row = data;
				for(j = 0; j < nth; j++)
				{
					row[0] = tb_r;
					row[1] = tb_g;
					row[2] = tb_b;
					row[3] = 0xff;
					row += bpr;
				}
			}

		}
		else
		{

			trough_color(1., rr, gg, bb);
			rr *= 255;
			gg *= 255;
			bb *= 255;
			rr = CLAMP((rr + b_r * 3) / 4, 0, 255);
			gg = CLAMP((gg + b_g * 3) / 4, 0, 255);
			bb = CLAMP((bb + b_b * 3) / 4, 0, 255);

			for(i = 0; i < ntw; i++)
			{
				unsigned char *row = data;
				float y = nth - histval[i];
				float y0 = i > 0 ? nth - histval[i - 1] : y;
				float y1 = i + 1 < ntw ? nth - histval[i + 1] : y;
				float yp = MIN(y, MIN(y0 + 1, y1 + 1));
				float y3 = (y0 + y + y1) * .333f;

				if(histred)
				{
					r = histred[i];
					g = histgreen[i];
					b = histblue[i];
				}
				else
				{
					float val = (i < t_w01 - 1 ?
						((MIN_HISTBRACKET / 100.f) - i / (t_w01 - 1.f) * (MIN_HISTBRACKET / 100.f)) :
						(i < t_w01 + t_w12 - 4 ?
						((i - t_w01 + 1) / (t_w12 - 4.f)) :
						((i - t_w01 - t_w12 + 5) / (t_w23 - 1.f) * ((MAX_HISTBRACKET / 100.f) - 1.f) + 1.f)));

					trough_color(val, r, g, b);
					r = CLAMP(r, 0, 255);
					g = CLAMP(g, 0, 255);
					b = CLAMP(b, 0, 255);

				}

				for(j = 0; j < yp - 1 && j < nth; j++)
				{
					row[0] = tb_r;
					row[1] = tb_g;
					row[2] = tb_b;
					row[3] = 0xff;
					row += bpr;
				}

				a= .75 * ha;
				for(; j < y && j < nth; j++)
				{
					row[0] = rint(rr * a + tb_r * (1 - a));
					row[1] = rint(gg * a + tb_g * (1 - a));
					row[2] = rint(bb * a + tb_b * (1 - a));
					row[3] = 0xff;
					row += bpr;
				}

				a = ha;
				for(; j < nth; j++)
				{
					float a = CLAMP(1. - .3 * (float)(j - y3) / (nth - y3 + 1), 0.f, 1.f) * ha;
					row[0] = rint(r * a * 255.f + tb_r * (1 - a));
					row[1] = rint(g * a * 255.f + tb_g * (1 - a));
					row[2] = rint(b * a * 255.f + tb_b * (1 - a));
					row[3] = 0xff;
					row += bpr;
				}

				data += bpp;
			}
		}

		draw_vframe(trough, ntx, nty, t_w01 - 2, 1, 0, 0, t_w01 - 2, 1);
		draw_vframe(trough, ntx, nty + 1, t_w01 - 1, nth - 2, 0, 1, t_w01 - 1, nth - 2);
		draw_vframe(trough, ntx, nty + nth - 1, t_w01 - 2, 1, 0,  nth - 1, t_w01 - 2, 1);

		draw_vframe(trough, t_x1 + 3, nty, t_w12 - 5, 1, t_w01, 0, t_w12 - 5, 1);
		draw_vframe(trough, t_x1 + 2, nty + 1, t_w12 - 3, nth - 2, t_w01 - 1, 1, t_w12 - 3, nth - 2);
		draw_vframe(trough, t_x1 + 3, nty + nth - 1, t_w12 - 5, 1, t_w01, nth - 1, t_w12 - 5, 1);

		draw_vframe(trough, t_x2 + 3, nty,  t_w23 - 2, 1, t_w01 + t_w12 - 3, 0, t_w23 - 2, 1);
		draw_vframe(trough, t_x2 + 2, nty + 1, t_w23 - 1, nth - 2, t_w01 + t_w12 - 4, 1, t_w23 - 1, nth - 2);
		draw_vframe(trough, t_x2 + 3, nty + nth - 1, t_w23 - 2, 1, t_w01 + t_w12 - 3, nth - 1, t_w23 - 2, 1);
	}

	draw_topslider(rint(gamma_to_pixel(gamma)), light == 3);

	draw_bottomslider(rint(value_to_pixel(loval)), light == 0);
	draw_bottomslider(rint(value_to_pixel(hival)), light == 2);
	draw_midslider(rint(value_to_pixel((loval + hival) / 2.)), light == 1);

	flash();
	flush();
}

void BluebananaSliderChannel::update_histogram(float *raw, float *red, float *green, float *blue, int n)
{
	int i, j;
	int h = troughlines;
	int w = troughcols - 6;
	float il2n = 1. / log2(n);
	int w0 = t_w01 - 1;
	int w1 = t_w12 - 4;
	int w2 = t_w23 - 1;

	if(!histval)
		histval = new float[w];

	if(!histred && red)
	{
		histred = new float[w];
		histgreen = new float[w];
		histblue = new float[w];
	}

	// resample left
	int span0 = rint((0.f - MIN_HISTBRACKET) / (MAX_HISTBRACKET - MIN_HISTBRACKET) * HISTSIZE);
	resample_histogram(raw, red, green, blue, histval, histred, histgreen, histblue,
		span0, 0, w0, 0, w0);

	// resample center
	int span1 = rint(100.f / (MAX_HISTBRACKET - MIN_HISTBRACKET) * HISTSIZE);
	resample_histogram(raw, red, green, blue, histval + w0, histred + w0, histgreen + w0, histblue + w0,
		span1, span0, w1, 0, w1 + 1);

	// resample right
	int span2 = rint((MAX_HISTBRACKET-100.f) / (MAX_HISTBRACKET-MIN_HISTBRACKET) * HISTSIZE);
	resample_histogram(raw, red, green, blue, histval + w0 + w1, histred + w0 + w1, histgreen + w0 + w1, histblue + w0 + w1,
		span2, span0 + span1, w2, 1, w2 + 1);

	// select value normalization
	for(i = 0; i < w; i++)
	{
		if(histval[i] <= .0001f)
		{
			histval[i] = -2;
		}
		else
		{
			float val = log2(histval[i]) * il2n;
			val = pow(val < 0 ? 0 : val, .75) * troughlines;
			if(val < 0)val = 0;
			if(val > troughlines) val = troughlines;
			histval[i] = val;
		}
	}
	render();
}

// ---------------------------- Fill ----------------------------------

int BluebananaSliderFill::over_to_pixel(float input)
{
	return fraction_to_pixel(input / maxover);
}

float BluebananaSliderFill::pixel_to_over(int pixel)
{
	return pixel_to_fraction(pixel) * maxover;
}

BluebananaSliderFill::BluebananaSliderFill(BluebananaMain *plugin,
	BluebananaWindow *gui,
	int x,
	int y,
	int w,
	int h,
	float minval,
	float maxval,
	float maxover)
 : BluebananaSlider(plugin,gui,x,y,w,h,minval,maxval)
{
	loval = -2;
	midval = 0;
	hival = 2;
	overval = 0;
	this->maxover = maxover;
}

int BluebananaSliderFill::button_press_event()
{
	if(is_event_win() && cursor_inside())
	{
		int min;
		int max;
		int cx = get_cursor_x();

		gui->deactivate();

		if(shift_down())
		{
			// narrow/widen range
			if(get_buttonpress() == 4)
			{
				set_delta(1);
				return 1;
			}
			else if(get_buttonpress() == 5)
			{
				set_delta(-1);
				return 1;
			}
		}
		else if (ctrl_down())
		{
			// shift all vals up/down
			if(get_buttonpress() == 4)
			{
				if(loval - increment > minval)
					set_fill(loval - 1, midval - 1, hival - 1);
				return 1;
			}
			else if(get_buttonpress() == 5)
			{
				if(hival + increment < maxval)
					set_fill(loval + 1, midval + 1, hival + 1);
				return 1;
			}
		}
		else
		{
			// increase/decrease feather
			if(get_buttonpress() == 4)
			{
				set_over(overval - 1);
				return 1;
			}
			else if(get_buttonpress() == 5)
			{
				set_over(overval + 1);
				return 1;
			}
		}

		switch(light)
		{
		case 0:
			xoff = cx - value_to_pixel(loval);
			break;
		case 1:
			xoff = cx - value_to_pixel(midval);
			break;
		case 2:
			xoff = cx - value_to_pixel(hival);
			break;
		case 3:
			xoff = cx - over_to_pixel(overval);
			break;
		default:
			return 1;
		}

		drag = 1;
		update();
		return 1;
	}
	return 0;
}

int BluebananaSliderFill::cursor_motion_event()
{
	int cx = get_cursor_x();
	int cy = get_cursor_y();

	if(drag)
	{
		switch(light)
		{
		case 0:
			set_fill(pixel_to_value(cx - xoff), midval, hival);
			break;
		case 1:
			set_fill(loval, pixel_to_value(cx - xoff), hival);
			break;
		case 2:
			set_fill(loval, midval, pixel_to_value(cx - xoff));
			break;
		case 3:
			set_over(pixel_to_over(cx - xoff));
			break;
		default:
			drag = 0;
			light = -1;
			update();
			break;
		}
		return 1;
	}
	else
	{
		if(is_event_win() && cursor_inside())
		{
			if(in_overslider(over_to_pixel(overval), cx, cy))
			{
				if(light != 3)
				{
					light = 3;
					update();
				}
			}
			else if(in_bottomslider(value_to_pixel(midval), cx, cy))
			{
				if(light != 1)
				{
					light = 1;
					update();
				}
			}
			else if(in_leftslider(value_to_pixel(loval), cx, cy))
			{
				if(light != 0)
				{
					light = 0;
					update();
				}
			}
			else if(in_rightslider(value_to_pixel(hival), cx, cy))
			{
				if(light != 2)
				{
					light = 2;
					update();
				}
			}
			else if(in_midslider2(value_to_pixel(midval), cx))
			{
				if(light != 1)
				{
					light = 1;
					update();
				}
			}
			else if(in_rightslider2(value_to_pixel(hival), cx))
			{
				if(light != 2)
				{
					light = 2;
					update();
				}
			}
			else if(in_leftslider2(value_to_pixel(loval), cx))
			{
				if(light != 0)
				{
					light = 0;
					update();
				}
			}
			else if(in_overslider2(over_to_pixel(overval), cx, cy))
			{
				if(light != 3)
				{
					light = 3;
					update();
				}
			}
			else
			{
				if(light != -1)
				{
					light = -1;
					update();
				}
			}
			return 1;
		}
		else
		{
			if(light != -1)
			{
				light = -1;
				update();
			}
		}
	}
	return 0;
}

void BluebananaSliderFill::set_fill(float lo, float mid, float hi)
{
	if(lo < minval) lo = minval;
	if(mid < minval) mid = minval;
	if(hi < 0) hi = 0;
	if(lo > 0) lo = 0;
	if(mid > maxval) mid = maxval;
	if(hi > maxval) hi = maxval;

	if(lo > midval) mid = lo;
	if(mid < loval) lo = mid;
	if(mid > hival) hi = mid;
	if(hi < midval) mid = hi;

	loval = lo;
	midval = mid;
	hival = hi;

	handle_event();
	update();
}

void BluebananaSliderFill::set_delta(float del)
{
	if(loval + del >= minval && hival + del <= maxval)
	{
		loval -= del;
		hival += del;
		if(loval > hival)
			loval = hival = rint((loval + hival) / 2);
		if(midval < loval) midval = loval;
		if(midval > hival)midval = hival;
		handle_event();
		update();
	}
}

void BluebananaSliderFill::set_over(float over)
{
	if(over >= 0 && over <= maxover)
	{
		overval = over;
		handle_event();
		update();
	}
}

void BluebananaSliderFill::update()
{
	int w = get_w();
	int h = get_h();
	int bg_c = get_resources()->get_bg_color();
	int bg_r = ((bg_c >> 16) & 0xff);
	int bg_g = ((bg_c >> 8) & 0xff);
	int bg_b = (bg_c & 0xff);
	int b_c = highlight ? active_bordercolor : inactive_bordercolor;
	int b_r = ((b_c >> 16) & 0xff);
	int b_g = ((b_c >> 8) & 0xff);
	int b_b = (b_c & 0xff);
	int tb_c = troughcolor;
	int tb_r = ((tb_c >> 16) & 0xff);
	int tb_g = ((tb_c >> 8) & 0xff);
	int tb_b = (tb_c & 0xff);

	int ZH = Z4 - Z1 + 1;
	int ZHC = Z4 - ZC + 1;

	int i, j;

	int xC = value_to_pixel(0);
	int ntw = troughcols;
	int nth = troughlines;
	int ntx = CW + ZL;
	int nty = Z1 + 1;

	clear_box(0, 0, w, h);

	// trough shadow

	// w+4 h+4 x-1 y-1
	float del = .9;
	int shadow_c = ((int)(bg_r * del) << 16) | ((int)(bg_g * del) << 8) | ((int)(bg_b * del));
	set_color(shadow_c);
	draw_box(0, Z1, xC + 3, ZH + 2);
	draw_box(1, Z1 - 1, xC + 1, ZH + 4);
	draw_box(xC, Z1, w - xC, ZH + 2);
	draw_box(xC + 1, Z1 -1, w - xC - 2, ZH + 4);

	// w+2 h+2 x y
	del = .8;
	shadow_c = ((int)(bg_r * del) << 16) | ((int)(bg_g * del) << 8) | ((int)(bg_b * del));
	set_color(shadow_c);
	draw_box(1, Z1 + 1, xC + 1, ZH);
	draw_box(2, Z1, xC - 1, ZH + 2);
	draw_box(xC + 1, Z1 + 1,w - xC - 2, ZH);
	draw_box(xC + 2, Z1, w - xC - 4, ZH + 2);

	// w h x+1 y+1
	del = .7;
	shadow_c = ((int)(bg_r * del) << 16) | ((int)(bg_g * del) << 8) | ((int)(bg_b * del));
	set_color(shadow_c);
	draw_box(2, Z1 + 2, xC - 1, ZH - 2);
	draw_box(3, Z1 + 1, xC - 3, ZH);
	draw_box(xC + 2, Z1 + 2, w - xC - 4, ZH - 2);
	draw_box(xC + 3, Z1 + 1, w - xC - 6, ZH);

	// trough border
	set_color(b_c);
	draw_box(1, Z1 + 1, xC - 1, ZH - 2);
	draw_box(2, Z1, xC - 3, ZH);
	draw_box(xC + 1, Z1 + 1, w - xC - 4, ZH - 2);
	draw_box(xC + 2, Z1, w - xC - 6, ZH);

	// trough inner
	// the trough in the two-part contour slider is slightly smaller than
	//   the full slider range, and cut into two sections
	if(!trough)
	{
		trough = new VFrame(NULL, ntw, nth);
	}

	/* Unlike the hist slider, we just drop the three pixel columns in the center */
	if(!trough)
	{
		fprintf(stderr, "Bluebanana: Unable to create Frame for slider\n");
	}
	else
	{
		unsigned char *data = trough->get_data();
		long bpr = trough->get_bytes_per_line();
		float ha = 1.;
		if(!highlight)
		{
			ha *= .25;
			tb_r = bg_r;
			tb_g = bg_g;
			tb_b = bg_b;
		}

		for(i = 0; i < ntw; i++)
		{
			unsigned char *row = data + i * trough->get_bytes_per_pixel();
			for(j = 0; j < nth; j++)
			{
				float r, g, b, a;
				trough_color((i + .5f) / ntw,(j + .5) / nth, r, g, b, a);
				r = CLAMP(r, 0, 255);
				g = CLAMP(g, 0, 255);
				b = CLAMP(b, 0, 255);
				a *= ha;
				row[0] = rint(r * a * 255 + tb_r * (1 - a));
				row[1] = rint(g * a * 255 + tb_g * (1 - a));
				row[2] = rint(b * a * 255 + tb_b * (1 - a));
				row[3] = 0xff;
				row += bpr;
			}
		}

		draw_vframe(trough, ntx, nty, xC - ntx - 2, 1, 0, 0, xC - ntx - 2, 1);
		draw_vframe(trough, ntx, nty + 1,  xC-ntx-1, nth-2, 0, 1, xC-ntx-1, nth-2);
		draw_vframe(trough, ntx, nty + nth - 1, xC-ntx-2, 1, 0, nth-1, xC-ntx-2, 1);

		draw_vframe(trough, xC + 3, nty, ntw - xC + ntx - 3, 1, xC - ntx + 2, 0, ntw - xC + ntx - 3, 1);
		draw_vframe(trough, xC + 2, nty + 1,  ntw - xC + ntx - 2, nth - 2, xC - ntx + 1, 1, ntx - xC + ntx - 2, nth - 2);
		draw_vframe(trough, xC + 3, nty + nth - 1, ntw-xC+ntx-3, 1, xC - ntx + 2, nth - 1, ntw - xC + ntx - 3, 1);
	}

	draw_overslider(over_to_pixel(overval), light == 3);
	draw_leftslider(value_to_pixel(loval), light == 0);
	draw_rightslider(value_to_pixel(hival), light == 2);
	draw_bottomslider(value_to_pixel(midval), light == 1);
	flash();
	flush();
}

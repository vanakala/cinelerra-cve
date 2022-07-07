// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcbitmap.h"
#include "bcpixmap.h"
#include "bcpopup.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bctimer.h"
#include "bcwindowbase.h"
#include "clip.h"
#include "colors.h"
#include "cursors.h"
#include "fonts.h"
#include "vframe.h"

#include <string.h>
#include <wchar.h>

void BC_WindowBase::copy_area(int x1, int y1, int x2, int y2, int w, int h,
	BC_Pixmap *pixmap)
{
	XCopyArea(top_level->display,
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
		top_level->gc, x1, y1, w, h, x2, y2);
}

void BC_WindowBase::draw_box(int x, int y, int w, int h, BC_Pixmap *pixmap)
{
	set_current_color();
	XFillRectangle(top_level->display,
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
		top_level->gc, x, y, w, h);
}


void BC_WindowBase::draw_circle(int x, int y, int w, int h, BC_Pixmap *pixmap)
{
	set_current_color();
	XDrawArc(top_level->display,
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
		top_level->gc, x, y, (w - 1), (h - 2), 0 * 64, 360 * 64);
}

void BC_WindowBase::draw_disc(int x, int y, int w, int h, BC_Pixmap *pixmap)
{
	set_current_color();
	XFillArc(top_level->display,
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
		top_level->gc, x, y, (w - 1), (h - 2), 0 * 64, 360 * 64);
}

void BC_WindowBase::clear_box(int x, int y, int w, int h, BC_Pixmap *pixmap)
{
	top_level->lock_window("BC_WindowBase::clear_box");
	set_current_color(bg_color);
	XFillRectangle(top_level->display,
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
		top_level->gc, x, y, w, h);
	top_level->unlock_window();
}

void BC_WindowBase::draw_text(int x, int y, const char *text, int length,
	BC_Pixmap *pixmap)
{
	if(length < 0)
		length = strlen(text);

	switch(top_level->current_font)
	{
	case MEDIUM_7SEGMENT:
		set_current_color();
		for(int i = 0; i < length; i++)
		{
			VFrame *image;
			switch(text[i])
			{
			case '0':
				image = resources.medium_7segment[0];
				break;
			case '1':
				image = resources.medium_7segment[1];
				break;
			case '2':
				image = resources.medium_7segment[2];
				break;
			case '3':
				image = resources.medium_7segment[3];
				break;
			case '4':
				image = resources.medium_7segment[4];
				break;
			case '5':
				image = resources.medium_7segment[5];
				break;
			case '6':
				image = resources.medium_7segment[6];
				break;
			case '7':
				image = resources.medium_7segment[7];
				break;
			case '8':
				image = resources.medium_7segment[8];
				break;
			case '9':
				image = resources.medium_7segment[9];
				break;
			case ':':
				image = resources.medium_7segment[10];
				break;
			case '.':
				image = resources.medium_7segment[11];
				break;
			case 'a':
			case 'A':
				image = resources.medium_7segment[12];
				break;
			case 'b':
			case 'B':
				image = resources.medium_7segment[13];
				break;
			case 'c':
			case 'C':
				image = resources.medium_7segment[14];
				break;
			case 'd':
			case 'D':
				image = resources.medium_7segment[15];
				break;
			case 'e':
			case 'E':
				image = resources.medium_7segment[16];
				break;
			case 'f':
			case 'F':
				image = resources.medium_7segment[17];
				break;
			case ' ':
				image = resources.medium_7segment[18];
				break;
			case '-':
				image = resources.medium_7segment[19];
				break;
			default:
				image = resources.medium_7segment[18];
				break;
			}

			draw_vframe(image, x, y - image->get_h());
			x += image->get_w();
		}
		break;

	default:
		if(top_level->get_xft_struct(top_level->current_font))
		{
			int l = resize_wide_text(length);

			length = BC_Resources::encode(BC_Resources::encoding,
				BC_Resources::wide_encoding,
				(char*)text, (char*)wide_text, l * sizeof(wchar_t),
				length) / sizeof(wchar_t);

			draw_wide_text(x, y, length, pixmap);
		}
	}
}

void BC_WindowBase::draw_text(int x, int y, const wchar_t *text, int length,
	BC_Pixmap *pixmap)
{
	if(length < 0)
		length = wcslen(text);

	resize_wide_text(length);
	wcscpy(wide_text, text);
	draw_wide_text(x, y, length, pixmap);
}

void BC_WindowBase::draw_utf8_text(int x, int y, const char *text, int length,
	BC_Pixmap *pixmap)
{
	if(length < 0)
		length = strlen(text);

	if(top_level->get_xft_struct(top_level->current_font))
	{
		int l = resize_wide_text(length);

		length = BC_Resources::encode("UTF8", BC_Resources::wide_encoding,
			(char*)text, (char*)wide_text, l * sizeof(wchar_t), length) / sizeof(wchar_t);

		draw_wide_text(x, y, length, pixmap);
	}
}

int BC_WindowBase::resize_wide_text(int length)
{
	int len;

	len = sizeof(wide_buffer) / sizeof(wchar_t);

	if(wide_text != wide_buffer)
		delete [] wide_text;

	if(length < len)
		wide_text = wide_buffer;
	else {
		wide_text = new wchar_t[length + 1];
		len = length + 1;
	}
	*wide_text = 0;
	return len;
}

void BC_WindowBase::draw_wide_text(int x, int y, int length, BC_Pixmap *pixmap)
{
	wchar_t *up, *upb, *tx_end;

	tx_end = &wide_text[length];

	for(upb = up = wide_text; up < tx_end; up++)
	{
		if(*up < ' ')
		{
			draw_wtext(x, y, upb, up - upb, pixmap);
			while(up < tx_end && *up == '\n')
			{
				y += get_text_height(top_level->current_font);
				up++;
			}
			upb = up;
		}
	}
	if(upb < up && up <= tx_end)
		draw_wtext(x, y, upb, up - upb, pixmap);
}

int BC_WindowBase::wcharpos(const wchar_t *text, XftFont *font, int length,
		int *charpos)
{
	XGlyphInfo extents;

	if(charpos)
	{
		int bpos = charpos[-1];

		for(int i = 0; i < length; i++)
		{
			XftTextExtents32(top_level->display, font,
				(const FcChar32*)text, i + 1,
				&extents);
			charpos[i] = extents.xOff + bpos;
		}
		return charpos[length - 1] - bpos;
	}
	else
	{
		XftTextExtents32(top_level->display, font,
			(const FcChar32*)text, length,
			&extents);
		return extents.xOff;
	}
}

void BC_WindowBase::draw_wtext(int x, int y, const wchar_t *text, int length,
	BC_Pixmap *pixmap, int *charpos)
{
	XRenderColor color;
	XftColor xft_color;
	const wchar_t *up, *ubp;
	int l, *cp;
	FcPattern *newpat;
	XftFont *curfont, *nextfont, *altfont, *basefont;

	if(length < 0)
		length = wcslen(text);

	if(charpos)
		charpos[0] = 0;
	if(!length)
		return;

	basefont = top_level->get_xft_struct(top_level->current_font);

	if(!basefont)
		return;

	top_level->lock_window("BC_WindowBase::draw_wtext");
	color.red = (current_color & 0xff0000) >> 16;
	color.red |= color.red << 8;
	color.green = (current_color & 0xff00) >> 8;
	color.green |= color.green << 8;
	color.blue = (current_color & 0xff);
	color.blue |= color.blue << 8;
	color.alpha = 0xffff;

	XftColorAllocValue(top_level->display, top_level->vis,
		top_level->cmap, &color, &xft_color);

	curfont = nextfont = basefont;
	altfont = 0;
	cp = 0;
	ubp = text;

	for(up = text; up < &text[length]; up++)
	{
		if(XftCharExists(top_level->display, basefont, *up))
			nextfont = basefont;
		else if(altfont && XftCharExists(top_level->display, altfont, *up))
			nextfont = altfont;
		else
		{
			if(altfont)
				XftFontClose(top_level->display, altfont);

			if(newpat = BC_Resources::find_similar_font(*up, basefont->pattern))
			{
				double psize;

				FcPatternGetDouble(basefont->pattern, FC_PIXEL_SIZE,
					0, &psize);
				FcPatternAddDouble(newpat, FC_PIXEL_SIZE, psize);
				FcPatternDel(newpat, FC_SCALABLE);
				altfont = XftFontOpenPattern(top_level->display,
					newpat);
				if(altfont)
					nextfont = altfont;
			}
			else
			{
				altfont = 0;
				nextfont = basefont;
			}
		}
		if(nextfont != curfont)
		{
			l = up - ubp;
			XftDrawString32((XftDraw*)(pixmap ?
				pixmap->opaque_xft_draw : this->pixmap->opaque_xft_draw),
				&xft_color, curfont, x, y,
				(const FcChar32*)ubp, l);

			if(charpos)
				cp = &charpos[ubp - text + 1];

			x += wcharpos(ubp, curfont, l, cp);
			ubp = up;
			curfont = nextfont;
		}
	}

	if(up > ubp)
	{
		XftDrawString32((XftDraw*)(pixmap ? pixmap->opaque_xft_draw :
			this->pixmap->opaque_xft_draw), &xft_color,
			curfont, x, y,
			(const FcChar32*)ubp, up - ubp);
		if(charpos)
			wcharpos(ubp, curfont, up - ubp, &charpos[ubp - text + 1]);
	}

	if(altfont)
		XftFontClose(top_level->display, altfont);

	XftColorFree(top_level->display, top_level->vis, top_level->cmap,
		&xft_color);
	top_level->unlock_window();
}

void BC_WindowBase::draw_center_text(int x, int y, const char *text, int length)
{
	int w, l;

	if(length < 0)
		length = strlen(text);

	if(top_level->get_xft_struct(top_level->current_font))
	{
		l = resize_wide_text(length);
		length = BC_Resources::encode(BC_Resources::encoding,
			BC_Resources::wide_encoding,
			(char*)text, (char*)wide_text, l * sizeof(wchar_t), length) /
			sizeof(wchar_t);
		w = get_text_width(current_font, wide_text, length);
		x -= w / 2;
		draw_wide_text(x, y, length, 0);
	}
}

void BC_WindowBase::draw_center_text(int x, int y, const wchar_t *text, int length)
{
	int w;

	if(length < 0) length = wcslen(text);

	if(top_level->get_xft_struct(top_level->current_font))
	{
		resize_wide_text(length);
		wcscpy(wide_text, text);
		w = get_text_width(current_font, wide_text, length);
		x -= w / 2;
		draw_wide_text(x, y, length, 0);
	}
}

void BC_WindowBase::draw_line(int x1, int y1, int x2, int y2, BC_Pixmap *pixmap)
{
	set_current_color();
	XDrawLine(top_level->display,
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
		top_level->gc, x1, y1, x2, y2);
}

void BC_WindowBase::draw_polygon(ArrayList<int> *x, ArrayList<int> *y, BC_Pixmap *pixmap)
{
	int npoints = MIN(x->total, y->total);
	XPoint *points = new XPoint[npoints];

	for(int i = 0; i < npoints; i++)
	{
		points[i].x = x->values[i];
		points[i].y = y->values[i];
	}

	set_current_color();
	XDrawLines(top_level->display,
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
		top_level->gc, points, npoints,
		CoordModeOrigin);

	delete [] points;
}


void BC_WindowBase::draw_rectangle(int x, int y, int w, int h)
{
	set_current_color();
	XDrawRectangle(top_level->display, pixmap->opaque_pixmap,
		top_level->gc, x, y, w - 1, h - 1);
}

void BC_WindowBase::draw_3d_border(int x, int y, int w, int h,
	int light1, int light2, int shadow1, int shadow2)
{
	int lx, ly, ux, uy;

	top_level->lock_window("BC_WindowBase::draw_3d_border");
	h--; w--;

	lx = x+1;  ly = y+1;
	ux = x+w-1;  uy = y+h-1;

	set_color(light1);
	draw_line(x, y, ux, y);
	draw_line(x, y, x, uy);
	set_color(light2);
	draw_line(lx, ly, ux - 1, ly);
	draw_line(lx, ly, lx, uy - 1);

	set_color(shadow1);
	draw_line(ux, ly, ux, uy);
	draw_line(lx, uy, ux, uy);
	set_color(shadow2);
	draw_line(x + w, y, x + w, y + h);
	draw_line(x, y + h, x + w, y + h);
	top_level->unlock_window();
}

void BC_WindowBase::draw_3d_box(int x, int y, int w, int h,
	int light1, int light2, int middle, int shadow1, int shadow2,
	BC_Pixmap *pixmap)
{
	int lx, ly, ux, uy;

	top_level->lock_window("BC_WindowBase::draw_3d_box");
	h--; w--;

	lx = x+1;  ly = y+1;
	ux = x+w-1;  uy = y+h-1;

	set_color(middle);
	draw_box(x, y, w, h, pixmap);

	set_color(light1);
	draw_line(x, y, ux, y, pixmap);
	draw_line(x, y, x, uy, pixmap);
	set_color(light2);
	draw_line(lx, ly, ux - 1, ly, pixmap);
	draw_line(lx, ly, lx, uy - 1, pixmap);

	set_color(shadow1);
	draw_line(ux, ly, ux, uy, pixmap);
	draw_line(lx, uy, ux, uy, pixmap);
	set_color(shadow2);
	draw_line(x + w, y, x + w, y + h, pixmap);
	draw_line(x, y + h, x + w, y + h, pixmap);
	top_level->unlock_window();
}

void BC_WindowBase::draw_colored_box(int x, int y, int w, int h,
	int down, int highlighted)
{
	if(!down)
	{
		if(highlighted)
			draw_3d_box(x, y, w, h,
				resources.button_light,
				resources.button_highlighted,
				resources.button_highlighted,
				resources.button_shadow,
				BLACK);
		else
			draw_3d_box(x, y, w, h,
				resources.button_light,
				resources.button_up,
				resources.button_up,
				resources.button_shadow,
				BLACK);
	}
	else
	{
// need highlighting for toggles
		if(highlighted)
			draw_3d_box(x, y, w, h,
				resources.button_shadow,
				BLACK,
				resources.button_up,
				resources.button_up,
				resources.button_light);
		else
			draw_3d_box(x, y, w, h,
				resources.button_shadow,
				BLACK,
				resources.button_down,
				resources.button_down,
				resources.button_light);
	}
}

void BC_WindowBase::draw_border(const char *text, int x, int y, int w, int h)
{
	int left_indent = 20;
	int lx, ly, ux, uy;

	top_level->lock_window("BC_WindowBase::draw_border");
	h--; w--;
	lx = x + 1;  ly = y + 1;
	ux = x + w - 1;  uy = y + h - 1;

	set_opaque();
	if(text && text[0] != 0)
	{
		set_color(BLACK);
		set_font(MEDIUMFONT);
		draw_text(x + left_indent, y + get_text_height(MEDIUMFONT) / 2, text);
	}

	set_color(resources.button_shadow);
	draw_line(x, y, x + left_indent - 5, y);
	draw_line(x, y, x, uy);
	draw_line(x + left_indent + 5 + get_text_width(MEDIUMFONT, text), y, ux, y);
	draw_line(x, y, x, uy);
	draw_line(ux, ly, ux, uy);
	draw_line(lx, uy, ux, uy);
	set_color(resources.button_light);
	draw_line(lx, ly, x + left_indent - 5 - 1, ly);
	draw_line(lx, ly, lx, uy - 1);
	draw_line(x + left_indent + 5 + get_text_width(MEDIUMFONT, text), ly, ux - 1, ly);
	draw_line(lx, ly, lx, uy - 1);
	draw_line(x + w, y, x + w, y + h);
	draw_line(x, y + h, x + w, y + h);
	top_level->unlock_window();
}

void BC_WindowBase::draw_triangle_down_flat(int x, int y, int w, int h)
{
	int x1, y1, x2, y2, x3, y3;
	XPoint point[3];

	top_level->lock_window("BC_WindowBase::draw_triangle_up");
	x1 = x; x2 = x + w / 2; x3 = x + w - 1;
	y1 = y; y2 = y + h - 1;

	point[0].x = x2; point[0].y = y2; point[1].x = x3;
	point[1].y = y1; point[2].x = x1; point[2].y = y1;

	set_current_color();
	XFillPolygon(top_level->display, pixmap->opaque_pixmap,
		top_level->gc, (XPoint *)point, 3, Nonconvex,
		CoordModeOrigin);
	top_level->unlock_window();
}

void BC_WindowBase::draw_triangle_up(int x, int y, int w, int h, 
	int light1, int light2, int middle, int shadow1, int shadow2)
{
	int x1, y1, x2, y2, x3, y3;
	XPoint point[3];

	top_level->lock_window("BC_WindowBase::draw_triangle_up");
	x1 = x; y1 = y; x2 = x + w / 2;
	y2 = y + h - 1; x3 = x + w - 1;

// middle
	point[0].x = x2; point[0].y = y1; point[1].x = x3;
	point[1].y = y2; point[2].x = x1; point[2].y = y2;

	set_current_color(middle);
	XFillPolygon(top_level->display, pixmap->opaque_pixmap,
		top_level->gc, (XPoint *)point, 3, Nonconvex,
		CoordModeOrigin);

// bottom and top right
	set_color(shadow1);
	draw_line(x3, y2-1, x1, y2-1);
	draw_line(x2-1, y1, x3-1, y2);
	set_color(shadow2);
	draw_line(x3, y2, x1, y2);
	draw_line(x2, y1, x3, y2);

// top left
	set_color(light2);
	draw_line(x2+1, y1, x1+1, y2);
	set_color(light1);
	draw_line(x2, y1, x1, y2);
	top_level->unlock_window();
}

void BC_WindowBase::draw_triangle_down(int x, int y, int w, int h, 
	int light1, int light2, int middle, int shadow1, int shadow2)
{
	int x1, y1, x2, y2, x3, y3;
	XPoint point[3];

	top_level->lock_window("BC_WindowBase::draw_triangle_down");
	x1 = x; x2 = x + w / 2; x3 = x + w - 1;
	y1 = y; y2 = y + h - 1;

	point[0].x = x2; point[0].y = y2; point[1].x = x3;
	point[1].y = y1; point[2].x = x1; point[2].y = y1;

	set_current_color(middle);
	XFillPolygon(top_level->display, pixmap->opaque_pixmap,
		top_level->gc, (XPoint *)point, 3, Nonconvex,
		CoordModeOrigin);

// top and bottom left
	set_color(light2);
	draw_line(x3-1, y1+1, x1+1, y1+1);
	draw_line(x1+1, y1, x2+1, y2);
	set_color(light1);
	draw_line(x3, y1, x1, y1);
	draw_line(x1, y1, x2, y2);

// bottom right
	set_color(shadow1);
	draw_line(x3-1, y1, x2-1, y2);
	set_color(shadow2);
	draw_line(x3, y1, x2, y2);
	top_level->unlock_window();
}

void BC_WindowBase::draw_triangle_left(int x, int y, int w, int h, 
	int light1, int light2, int middle, int shadow1, int shadow2)
{
	int x1, y1, x2, y2, x3, y3;
	XPoint point[3];

	top_level->lock_window("BC_WindowBase::draw_triangle_left");
	// draw back arrow
	y1 = y; x1 = x; y2 = y + h / 2;
	x2 = x + w - 1; y3 = y + h - 1;

	point[0].x = x1; point[0].y = y2; point[1].x = x2; 
	point[1].y = y1; point[2].x = x2; point[2].y = y3;

	set_current_color(middle);
	XFillPolygon(top_level->display, pixmap->opaque_pixmap,
		top_level->gc, (XPoint *)point, 3, Nonconvex,
		CoordModeOrigin);

// right and bottom right
	set_color(shadow1);
	draw_line(x2-1, y1, x2-1, y3-1);
	draw_line(x2, y3-1, x1, y2-1);
	set_color(shadow2);
	draw_line(x2, y1, x2, y3);
	draw_line(x2, y3, x1, y2);

// top left
	set_color(light1);
	draw_line(x1, y2, x2, y1);
	set_color(light2);
	draw_line(x1, y2+1, x2, y1+1);
	top_level->unlock_window();
}

void BC_WindowBase::draw_triangle_right(int x, int y, int w, int h, 
	int light1, int light2, int middle, int shadow1, int shadow2)
{
	int x1, y1, x2, y2, x3, y3;
	XPoint point[3];

	top_level->lock_window("BC_WindowBase::draw_triangle_left");
	y1 = y; y2 = y + h / 2; y3 = y + h - 1; 
	x1 = x; x2 = x + w - 1;

	point[0].x = x1; point[0].y = y1; point[1].x = x2; 
	point[1].y = y2; point[2].x = x1; point[2].y = y3;

	set_current_color(middle);
	XFillPolygon(top_level->display, pixmap->opaque_pixmap,
		top_level->gc, (XPoint *)point, 3, Nonconvex,
		CoordModeOrigin);

// left and top right
	set_color(light2);
	draw_line(x1+1, y3, x1+1, y1);
	draw_line(x1, y1+1, x2, y2+1);
	set_color(light1);
	draw_line(x1, y3, x1, y1);
	draw_line(x1, y1, x2, y2);

// bottom right
	set_color(shadow1);
	draw_line(x2, y2-1, x1, y3-1);
	set_color(shadow2);
	draw_line(x2, y2, x1, y3);
	top_level->unlock_window();
}

void BC_WindowBase::draw_check(int x, int y)
{
	const int w = 15, h = 15;

	top_level->lock_window("BC_WindowBase::draw_check");
	draw_line(x + 3, y + h / 2 + 0, x + 6, y + h / 2 + 2);
	draw_line(x + 3, y + h / 2 + 1, x + 6, y + h / 2 + 3);
	draw_line(x + 6, y + h / 2 + 2, x + w - 4, y + h / 2 - 3);
	draw_line(x + 3, y + h / 2 + 2, x + 6, y + h / 2 + 4);
	draw_line(x + 6, y + h / 2 + 2, x + w - 4, y + h / 2 - 3);
	draw_line(x + 6, y + h / 2 + 3, x + w - 4, y + h / 2 - 2);
	draw_line(x + 6, y + h / 2 + 4, x + w - 4, y + h / 2 - 1);
	top_level->unlock_window();
}

void BC_WindowBase::draw_tiles(BC_Pixmap *tile, int origin_x, int origin_y, int x, int y, int w, int h)
{
	if(!tile)
	{
		set_color(bg_color);
		draw_box(x, y, w, h);
	}
	else
	{
		XSetFillStyle(top_level->display, top_level->gc, FillTiled);
// Don't know how slow this is
		XSetTile(top_level->display, top_level->gc, tile->get_pixmap());
		XSetTSOrigin(top_level->display, top_level->gc, origin_x, origin_y);
		draw_box(x, y, w, h);
		XSetFillStyle(top_level->display, top_level->gc, FillSolid);
	}
}

void BC_WindowBase::draw_top_tiles(BC_WindowBase *parent_window, int x, int y, int w, int h)
{
	Window tempwin;
	int origin_x, origin_y;

	XTranslateCoordinates(top_level->display, parent_window->win, win,
		0, 0, &origin_x, &origin_y, &tempwin);
	draw_tiles(parent_window->bg_pixmap, origin_x, origin_y, x, y, w, h);
}

void BC_WindowBase::draw_top_background(BC_WindowBase *parent_window,
	int x, int y, int w, int h, BC_Pixmap *pixmap)
{
	Window tempwin;
	int top_x, top_y;

	XTranslateCoordinates(top_level->display, win, parent_window->win,
		x, y, &top_x, &top_y, &tempwin);

	XCopyArea(top_level->display, parent_window->pixmap->opaque_pixmap,
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
		top_level->gc, top_x, top_y, w, h, x, y);
}

void BC_WindowBase::draw_background(int x, int y, int w, int h)
{
	if(bg_pixmap)
		draw_tiles(bg_pixmap, 0, 0, x, y, w, h);
	else
		clear_box(x, y, w, h);
}

void BC_WindowBase::draw_bitmap(BC_Bitmap *bitmap,
	int dest_x, int dest_y,int dest_w, int dest_h,
	int src_x, int src_y, int src_w, int src_h,
	BC_Pixmap *pixmap)
{

// Hide cursor if video enabled
	update_video_cursor();

	if(dest_w <= 0 || dest_h <= 0)
	{
// Use hardware scaling to canvas dimensions if proper color model.
		if(bitmap->get_color_model() == BC_YUV420P)
		{
			dest_w = w;
			dest_h = h;
		}
		else
		{
			dest_w = bitmap->get_w();
			dest_h = bitmap->get_h();
		}
	}

	if(src_w <= 0 || src_h <= 0)
	{
		src_w = bitmap->get_w();
		src_h = bitmap->get_h();
	}

	if(video_on)
	{
		bitmap->write_drawable(win, top_level->gc,
			src_x, src_y, src_w, src_h,
			dest_x, dest_y, dest_w, dest_h);
	}
	else
	{
		bitmap->write_drawable(pixmap ?
			pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
			top_level->gc, dest_x, dest_y, src_x, src_y,
			dest_w, dest_h);
	}
}

void BC_WindowBase::draw_pixel(int x, int y, BC_Pixmap *pixmap)
{
	XDrawPoint(top_level->display,
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
		top_level->gc, x, y);
}


void BC_WindowBase::draw_pixmap(BC_Pixmap *pixmap, 
	int dest_x, int dest_y, int dest_w, int dest_h,
	int src_x, int src_y, BC_Pixmap *dst)
{
	pixmap->write_drawable(dst ? dst->opaque_pixmap : this->pixmap->opaque_pixmap,
		dest_x, dest_y, dest_w, dest_h, src_x, src_y);
}

void BC_WindowBase::draw_vframe(VFrame *frame,
	int dest_x, int dest_y, int dest_w, int dest_h,
	int src_x, int src_y, int src_w, int src_h,
	BC_Pixmap *pixmap)
{
	if(dest_w <= 0)
		dest_w = frame->get_w() - src_x;
	if(dest_h <= 0)
		dest_h = frame->get_h() - src_y;
	if(src_w <= 0)
		src_w = frame->get_w() - src_x;
	if(src_h <= 0)
		src_h = frame->get_h() - src_y;
	CLAMP(src_x, 0, frame->get_w() - 1);
	CLAMP(src_y, 0, frame->get_h() - 1);
	if(src_x + src_w > frame->get_w())
		src_w = frame->get_w() - src_x;
	if(src_y + src_h > frame->get_h())
		src_h = frame->get_h() - src_y;

	if(!temp_bitmap)
		temp_bitmap = new BC_Bitmap(this, 0, 0,
			get_color_model(), 0);

	temp_bitmap->read_frame(frame,
		src_x, src_y, src_w, src_h,
		0, 0, dest_w, dest_h,
		0, get_color_model());

	draw_bitmap(temp_bitmap, dest_x, dest_y, dest_w, dest_h,
		0, 0, -1, -1, pixmap);
}

void BC_WindowBase::draw_tooltip()
{
	if(tooltip_popup)
	{
		int w = tooltip_popup->get_w(), h = tooltip_popup->get_h();

		top_level->lock_window("BC_WindowBase::draw_tooltip");
		tooltip_popup->set_color(resources.tooltip_bg_color);
		tooltip_popup->draw_box(0, 0, w, h);
		tooltip_popup->set_color(BLACK);
		tooltip_popup->draw_rectangle(0, 0, w, h);
		tooltip_popup->set_font(MEDIUMFONT);
		tooltip_popup->draw_text(TOOLTIP_MARGIN, 
			get_text_ascent(MEDIUMFONT) + TOOLTIP_MARGIN, 
			tooltip_wtext, tooltip_length);
		top_level->unlock_window();
	}
}

void BC_WindowBase::slide_left(int distance)
{
	if(distance < w)
	{
		XCopyArea(top_level->display, pixmap->opaque_pixmap,
			pixmap->opaque_pixmap, top_level->gc, distance,
			0, w - distance, h, 0, 0);
	}
}

void BC_WindowBase::slide_right(int distance)
{
	if(distance < w)
	{
		XCopyArea(top_level->display, pixmap->opaque_pixmap,
			pixmap->opaque_pixmap, top_level->gc, 0, 0,
			w - distance, h, distance, 0);
	}
}

void BC_WindowBase::slide_up(int distance)
{
	if(distance < h)
	{
		top_level->lock_window("BC_WindowBase::slide_up");
		XCopyArea(top_level->display, pixmap->opaque_pixmap,
			pixmap->opaque_pixmap, top_level->gc,
			0, distance, w, h - distance, 0, 0);
		set_current_color(bg_color);
		XFillRectangle(top_level->display, pixmap->opaque_pixmap,
			top_level->gc, 0, h - distance, w, distance);
		top_level->unlock_window();
	}
}

void BC_WindowBase::slide_down(int distance)
{
	if(distance < h)
	{
		top_level->lock_window("BC_WindowBase::slide_down");
		XCopyArea(top_level->display, pixmap->opaque_pixmap,
			pixmap->opaque_pixmap, top_level->gc, 0, 0,
			w, h - distance, 0, distance);
		set_current_color(bg_color);
		XFillRectangle(top_level->display, pixmap->opaque_pixmap,
			top_level->gc, 0, 0, w, distance);
		top_level->unlock_window();
	}
}


// Draw all 3 segments in a single vframe for a changing level

// total_x 
// <------>
// total_w
//         <------------------------------------------------------------>
// x
// |
// w           
// <-------------------------------------------------------------------->
// output
//         |-------------------|----------------------|------------------|

void BC_WindowBase::draw_3segmenth(int x, int y, int w, VFrame *image,
	BC_Pixmap *pixmap)
{
	draw_3segmenth(x, y, w, x, w, image, pixmap);
}

void BC_WindowBase::draw_3segmenth(int x, int y, int w, int total_x, int total_w,
	VFrame *image, BC_Pixmap *pixmap)
{
	if(total_w <= 0 || w <= 0 || h <= 0)
		return;

	int third_image = image->get_w() / 3;
	int half_image = image->get_w() / 2;
	int left_boundary = third_image;
	int right_boundary = total_w - third_image;
	int left_in_x = 0;
	int left_in_w = third_image;
	int left_out_x = total_x;
	int left_out_w = third_image;
	int right_in_x = image->get_w() - third_image;
	int right_in_w = third_image;
	int right_out_x = total_x + total_w - third_image;
	int right_out_w = third_image;
	int center_out_x = total_x + third_image;
	int center_out_w = total_w - third_image * 2;
	int image_x, image_w;

	if(left_out_x < x)
	{
		left_in_w -= x - left_out_x;
		left_out_w -= x - left_out_x;
		left_in_x += x - left_out_x;
		left_out_x += x - left_out_x;
	}

	if(left_out_x + left_out_w > x + w)
	{
		left_in_w -= (left_out_x + left_out_w) - (x + w);
		left_out_w -= (left_out_x + left_out_w) - (x + w);
	}

	if(right_out_x < x)
	{
		right_in_w -= x - right_out_x;
		right_out_w -= x - right_out_x;
		right_in_x += x - right_out_x;
		right_out_x += x - right_out_x;
	}

	if(right_out_x + right_out_w > x + w)
	{
		right_in_w -= (right_out_x + right_out_w) - (x + w);
		right_out_w -= (right_out_x + right_out_w) - (x + w);
	}

	if(center_out_x < x)
	{
		center_out_w -= x - center_out_x;
		center_out_x += x - center_out_x;
	}

	if(center_out_x + center_out_w > x + w)
		center_out_w -= (center_out_x + center_out_w) - (x + w);

	if(!temp_bitmap)
		temp_bitmap = new BC_Bitmap(top_level, image->get_w(), image->get_h(),
			get_color_model(), 0);

	temp_bitmap->read_frame(image, 0, 0, image->get_w(), image->get_h(),
		0, get_color_model());

// src width and height are meaningless in video_off mode
	if(left_out_w > 0)
		draw_bitmap(temp_bitmap, left_out_x, y, left_out_w, image->get_h(),
			left_in_x, 0, -1, -1, pixmap);

	if(right_out_w > 0)
		draw_bitmap(temp_bitmap, right_out_x, y, right_out_w, image->get_h(),
			right_in_x, 0, -1, -1, pixmap);

	for(int pixel = center_out_x; pixel < center_out_x + center_out_w;
			pixel += half_image)
	{
		int fragment_w = half_image;

		if(fragment_w + pixel > center_out_x + center_out_w)
			fragment_w = (center_out_x + center_out_w) - pixel;

		draw_bitmap(temp_bitmap, pixel, y,fragment_w, image->get_h(),
			third_image, 0, -1, -1, pixmap);
	}
}

void BC_WindowBase::draw_3segmenth(int x, int y, int w,
	int total_x, int total_w, BC_Pixmap *src, BC_Pixmap *dst)
{
	if(w <= 0 || total_w <= 0)
		return;

	int quarter_src = src->get_w() / 4;
	int half_src = src->get_w() / 2;
	int left_boundary = quarter_src;
	int right_boundary = total_w - quarter_src;
	int left_in_x = 0;
	int left_in_w = quarter_src;
	int left_out_x = total_x;
	int left_out_w = quarter_src;
	int right_in_x = src->get_w() - quarter_src;
	int right_in_w = quarter_src;
	int right_out_x = total_x + total_w - quarter_src;
	int right_out_w = quarter_src;
	int center_out_x = total_x + quarter_src;
	int center_out_w = total_w - quarter_src * 2;
	int src_x, src_w;

	if(left_out_x < x)
	{
		left_in_w -= x - left_out_x;
		left_out_w -= x - left_out_x;
		left_in_x += x - left_out_x;
		left_out_x += x - left_out_x;
	}

	if(left_out_x + left_out_w > x + w)
	{
		left_in_w -= (left_out_x + left_out_w) - (x + w);
		left_out_w -= (left_out_x + left_out_w) - (x + w);
	}

	if(right_out_x < x)
	{
		right_in_w -= x - right_out_x;
		right_out_w -= x - right_out_x;
		right_in_x += x - right_out_x;
		right_out_x += x - right_out_x;
	}

	if(right_out_x + right_out_w > x + w)
	{
		right_in_w -= (right_out_x + right_out_w) - (x + w);
		right_out_w -= (right_out_x + right_out_w) - (x + w);
	}

	if(center_out_x < x)
	{
		center_out_w -= x - center_out_x;
		center_out_x += x - center_out_x;
	}

	if(center_out_x + center_out_w > x + w)
	{
		center_out_w -= (center_out_x + center_out_w) - (x + w);
	}

	if(left_out_w > 0)
		draw_pixmap(src, left_out_x, y, left_out_w,
			src->get_h(), left_in_x, 0, dst);

	if(right_out_w > 0)
		draw_pixmap(src, right_out_x, y, right_out_w, src->get_h(),
			right_in_x, 0, dst);

	for(int pixel = center_out_x; pixel < center_out_x + center_out_w;
			pixel += half_src)
	{
		int fragment_w = half_src;

		if(fragment_w + pixel > center_out_x + center_out_w)
			fragment_w = (center_out_x + center_out_w) - pixel;

		draw_pixmap(src, pixel, y, fragment_w,
			src->get_h(), quarter_src, 0, dst);
	}

}

void BC_WindowBase::draw_3segmenth(int x, int y, int w,
	BC_Pixmap *src, BC_Pixmap *dst)
{
	if(w <= 0)
		return;

	int third_image = src->get_w() / 3;
	int half_output = w / 2;
	int left_boundary = third_image;
	int right_boundary = w - third_image;
	int left_in_x = 0;
	int left_in_w = third_image;
	int left_out_x = x;
	int left_out_w = third_image;
	int right_in_x = src->get_w() - third_image;
	int right_in_w = third_image;
	int right_out_x = x + w - third_image;
	int right_out_w = third_image;
	int image_x, image_w;

	if(left_out_w > half_output)
	{
		left_in_w -= left_out_w - half_output;
		left_out_w -= left_out_w - half_output;
	}

	if(right_out_x < x + half_output)
	{
		right_in_w -= x + half_output - right_out_x;
		right_out_w -= x + half_output - right_out_x;
		right_in_x += x + half_output - right_out_x;
		right_out_x += x + half_output - right_out_x;
	}

	if(left_out_w > 0)
		draw_pixmap(src, left_out_x, y, left_out_w, src->get_h(),
			left_in_x, 0, dst);

	if(right_out_w > 0)
		draw_pixmap(src, right_out_x, y, right_out_w, src->get_h(),
			right_in_x, 0, dst);

	for(int pixel = left_out_x + left_out_w; pixel < right_out_x;
			pixel += third_image)
	{
		int fragment_w = third_image;

		if(fragment_w + pixel > right_out_x)
			fragment_w = right_out_x - pixel;

		draw_pixmap(src, pixel, y, fragment_w, src->get_h(),
			third_image, 0, dst);
	}
}

void BC_WindowBase::draw_3segmentv(int x, int y, int h,
	VFrame *src, BC_Pixmap *dst)
{
	if(h <= 0)
		return;

	int third_image = src->get_h() / 3;
	int half_output = h / 2;
	int left_boundary = third_image;
	int right_boundary = h - third_image;
	int left_in_y = 0;
	int left_in_h = third_image;
	int left_out_y = y;
	int left_out_h = third_image;
	int right_in_y = src->get_h() - third_image;
	int right_in_h = third_image;
	int right_out_y = y + h - third_image;
	int right_out_h = third_image;
	int image_y, image_h;

	if(left_out_h > half_output)
	{
		left_in_h -= left_out_h - half_output;
		left_out_h -= left_out_h - half_output;
	}

	if(right_out_y < y + half_output)
	{
		right_in_h -= y + half_output - right_out_y;
		right_out_h -= y + half_output - right_out_y;
		right_in_y += y + half_output - right_out_y;
		right_out_y += y + half_output - right_out_y;
	}

	if(!temp_bitmap)
		temp_bitmap = new BC_Bitmap(top_level, src->get_w(), src->get_h(),
			get_color_model(), 0);

	temp_bitmap->read_frame(src, 0, 0, src->get_w(), src->get_h(),
		0, get_color_model());

	if(left_out_h > 0)
		draw_bitmap(temp_bitmap, x, left_out_y, src->get_w(),
			left_out_h, 0, left_in_y, -1, -1, dst);

	if(right_out_h > 0)
		draw_bitmap(temp_bitmap, x, right_out_y, src->get_w(),
			right_out_h, 0, right_in_y, -1, -1, dst);

	for(int pixel = left_out_y + left_out_h; pixel < right_out_y;
			pixel += third_image)
	{
		int fragment_h = third_image;

		if(fragment_h + pixel > right_out_y)
			fragment_h = right_out_y - pixel;

		draw_bitmap(temp_bitmap, x, pixel, src->get_w(), fragment_h,
			0, third_image, -1, -1, dst);
	}
}

void BC_WindowBase::draw_3segmentv(int x, int y, int h,
	BC_Pixmap *src, BC_Pixmap *dst)
{
	if(h <= 0)
		return;

	int third_image = src->get_h() / 3;
	int half_output = h / 2;
	int left_boundary = third_image;
	int right_boundary = h - third_image;
	int left_in_y = 0;
	int left_in_h = third_image;
	int left_out_y = y;
	int left_out_h = third_image;
	int right_in_y = src->get_h() - third_image;
	int right_in_h = third_image;
	int right_out_y = y + h - third_image;
	int right_out_h = third_image;
	int image_y, image_h;

	if(left_out_h > half_output)
	{
		left_in_h -= left_out_h - half_output;
		left_out_h -= left_out_h - half_output;
	}

	if(right_out_y < y + half_output)
	{
		right_in_h -= y + half_output - right_out_y;
		right_out_h -= y + half_output - right_out_y;
		right_in_y += y + half_output - right_out_y;
		right_out_y += y + half_output - right_out_y;
	}

	if(left_out_h > 0)
		draw_pixmap(src, x, left_out_y, src->get_w(), left_out_h,
			0, left_in_y, dst);

	if(right_out_h > 0)
		draw_pixmap(src, x, right_out_y, src->get_w(), right_out_h,
			0, right_in_y, dst);

	for(int pixel = left_out_y + left_out_h; pixel < right_out_y;
			pixel += third_image)
	{
		int fragment_h = third_image;

		if(fragment_h + pixel > right_out_y)
			fragment_h = right_out_y - pixel;

		draw_pixmap(src, x, pixel, src->get_w(), fragment_h,
			0, third_image, dst);
	}
}

void BC_WindowBase::draw_9segment(int x, int y, int w, int h,
	BC_Pixmap *src, BC_Pixmap *dst)
{
	if(w <= 0 || h <= 0)
		return;

	int in_x_third = src->get_w() / 3;
	int in_y_third = src->get_h() / 3;
	int out_x_half = w / 2;
	int out_y_half = h / 2;

	int in_x1 = 0;
	int in_y1 = 0;
	int out_x1 = 0;
	int out_y1 = 0;
	int in_x2 = MIN(in_x_third, out_x_half);
	int in_y2 = MIN(in_y_third, out_y_half);
	int out_x2 = in_x2;
	int out_y2 = in_y2;

	int out_x3 = MAX(w - out_x_half, w - in_x_third);
	int out_x4 = w;
	int in_x3 = src->get_w() - (out_x4 - out_x3);
	int in_x4 = src->get_w();

	int out_y3 = MAX(h - out_y_half, h - in_y_third);
	int out_y4 = h;
	int in_y3 = src->get_h() - (out_y4 - out_y3);
	int in_y4 = src->get_h();

// Segment 1
	draw_pixmap(src, x + out_x1, y + out_y1,
		out_x2 - out_x1, out_y2 - out_y1,
		in_x1, in_y1, dst);

// Segment 2 * n
	for(int i = out_x2; i < out_x3; i += in_x3 - in_x2)
	{
		if(out_x3 - i > 0)
		{
			int w = MIN(in_x3 - in_x2, out_x3 - i);
			draw_pixmap(src, x + i, y + out_y1, w,
				out_y2 - out_y1, in_x2, in_y1, dst);
		}
	}

// Segment 3
	draw_pixmap(src, x + out_x3, y + out_y1,
		out_x4 - out_x3, out_y2 - out_y1,
		in_x3, in_y1, dst);

// Segment 4 * n
	for(int i = out_y2; i < out_y3; i += in_y3 - in_y2)
	{
		if(out_y3 - i > 0)
		{
			int h = MIN(in_y3 - in_y2, out_y3 - i);

			draw_pixmap(src, x + out_x1, y + i,
				out_x2 - out_x1, h,
				in_x1, in_y2, dst);
		}
	}

// Segment 5 * n * n
	for(int i = out_y2; i < out_y3; i += in_y3 - in_y2)
	{
		if(out_y3 - i > 0)
		{
			int h = MIN(in_y3 - in_y2 /* in_y_third */, out_y3 - i);

			for(int j = out_x2; j < out_x3; j += in_x3 - in_x2)
			{
				int w = MIN(in_x3 - in_x2, out_x3 - j);

				if(out_x3 - j > 0)
					draw_pixmap(src, x + j, y + i, w, h,
						in_x2, in_y2, dst);
			}
		}
	}

// Segment 6 * n
	for(int i = out_y2; i < out_y3; i += in_y3 - in_y2)
	{
		if(out_y3 - i > 0)
		{
			int h = MIN(in_y3 - in_y2, out_y3 - i);

			draw_pixmap(src, x + out_x3, y + i, out_x4 - out_x3,
				h, in_x3, in_y2, dst);
		}
	}

// Segment 7
	draw_pixmap(src, x + out_x1, y + out_y3, out_x2 - out_x1,
		out_y4 - out_y3, in_x1, in_y3, dst);

// Segment 8 * n
	for(int i = out_x2; i < out_x3; i += in_x3 - in_x2)
	{
		if(out_x3 - i > 0)
		{
			int w = MIN(in_x3 - in_y2, out_x3 - i);

			draw_pixmap(src, x + i, y + out_y3, w, out_y4 - out_y3,
				in_x2, in_y3, dst);
		}
	}

// Segment 9
	draw_pixmap(src, x + out_x3, y + out_y3,
		out_x4 - out_x3, out_y4 - out_y3,
		in_x3, in_y3, dst);
}

void BC_WindowBase::draw_9segment(int x, int y, int w, int h,
	VFrame *src, BC_Pixmap *dst)
{
	if(w <= 0 || h <= 0)
		return;

	int in_x_third = src->get_w() / 3;
	int in_y_third = src->get_h() / 3;
	int out_x_half = w / 2;
	int out_y_half = h / 2;

	int in_x1 = 0;
	int in_y1 = 0;
	int out_x1 = 0;
	int out_y1 = 0;
	int in_x2 = MIN(in_x_third, out_x_half);
	int in_y2 = MIN(in_y_third, out_y_half);
	int out_x2 = in_x2;
	int out_y2 = in_y2;

	int out_x3 = MAX(w - out_x_half, w - in_x_third);
	int out_x4 = w;
	int in_x3 = src->get_w() - (out_x4 - out_x3);
	int in_x4 = src->get_w();

	int out_y3 = MAX(h - out_y_half, h - in_y_third);
	int out_y4 = h;
	int in_y3 = src->get_h() - (out_y4 - out_y3);
	int in_y4 = src->get_h();

	if(!temp_bitmap)
		temp_bitmap = new BC_Bitmap(top_level, src->get_w(), src->get_h(),
			get_color_model(), 0);

	temp_bitmap->read_frame(src, 0, 0, src->get_w(), src->get_h(),
		0, get_color_model());

// Segment 1
	draw_bitmap(temp_bitmap, x + out_x1, y + out_y1, out_x2 - out_x1,
		out_y2 - out_y1, in_x1, in_y1, in_x2 - in_x1,
		in_y2 - in_y1,  dst);

// Segment 2 * n
	for(int i = out_x2; i < out_x3; i += in_x3 - in_x2)
	{
		if(out_x3 - i > 0)
		{
			int w = MIN(in_x3 - in_x2, out_x3 - i);

			draw_bitmap(temp_bitmap, x + i, y + out_y1,
				w, out_y2 - out_y1,
				in_x2, in_y1, w,
				in_y2 - in_y1, dst);
		}
	}

// Segment 3
	draw_bitmap(temp_bitmap, x + out_x3, y + out_y1,
		out_x4 - out_x3, out_y2 - out_y1,
		in_x3, in_y1, in_x4 - in_x3, in_y2 - in_y1, dst);

// Segment 4 * n
	for(int i = out_y2; i < out_y3; i += in_y3 - in_y2)
	{
		if(out_y3 - i > 0)
		{
			int h = MIN(in_y3 - in_y2, out_y3 - i);

			draw_bitmap(temp_bitmap, x + out_x1, y + i,
				out_x2 - out_x1, h,
				in_x1, in_y2, in_x2 - in_x1, h, dst);
		}
	}

// Segment 5 * n * n
	for(int i = out_y2; i < out_y3; i += in_y3 - in_y2)
	{
		if(out_y3 - i > 0)
		{
			int h = MIN(in_y3 - in_y2, out_y3 - i);

			for(int j = out_x2; j < out_x3; j += in_x3 - in_x2)
			{
				int w = MIN(in_x3 - in_x2, out_x3 - j);

				if(out_x3 - j > 0)
					draw_bitmap(temp_bitmap,
						x + j, y + i, w, h,
						in_x2, in_y2, w, h, dst);
			}
		}
	}

// Segment 6 * n
	for(int i = out_y2; i < out_y3; i += in_y_third)
	{
		if(out_y3 - i > 0)
		{
			int h = MIN(in_y_third, out_y3 - i);
			draw_bitmap(temp_bitmap, x + out_x3, y + i,
				out_x4 - out_x3, h,
				in_x3, in_y2, in_x4 - in_x3, h, dst);
		}
	}

// Segment 7
	draw_bitmap(temp_bitmap, x + out_x1, y + out_y3, out_x2 - out_x1,
		out_y4 - out_y3, in_x1, in_y3, in_x2 - in_x1,
		in_y4 - in_y3, dst);

// Segment 8 * n
	for(int i = out_x2; i < out_x3; i += in_x_third)
	{
		if(out_x3 - i > 0)
		{
			int w = MIN(in_x_third, out_x3 - i);
			draw_bitmap(temp_bitmap, x + i, y + out_y3,
				w, out_y4 - out_y3,
				in_x2, in_y3, w,
				in_y4 - in_y3, dst);
		}
	}

// Segment 9
	draw_bitmap(temp_bitmap, x + out_x3, y + out_y3,
		out_x4 - out_x3, out_y4 - out_y3,
		in_x3, in_y3, in_x4 - in_x3, in_y4 - in_y3, dst);
}

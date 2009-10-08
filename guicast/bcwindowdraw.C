
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

#include "bcbitmap.h"
#include "bcpixmap.h"
#include "bcpopup.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcsynchronous.h"
#include "bctimer.h"
#include "bcwindowbase.h"
#include "clip.h"
#include "colors.h"
#include "cursors.h"
#include "fonts.h"
#include "vframe.h"
#include <string.h>

void BC_WindowBase::copy_area(int x1, int y1, int x2, int y2, int w, int h, BC_Pixmap *pixmap)
{
	XCopyArea(top_level->display, 
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap, 
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap, 
		top_level->gc, 
		x1, 
		y1, 
		w,
       	h, 
		x2, 
		y2);
}


void BC_WindowBase::draw_box(int x, int y, int w, int h, BC_Pixmap *pixmap)
{
//if(x == 0) printf("BC_WindowBase::draw_box %d %d %d %d\n", x, y, w, h);
	XFillRectangle(top_level->display, 
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap, 
		top_level->gc, 
		x, 
		y, 
		w, 
		h);
}


void BC_WindowBase::draw_circle(int x, int y, int w, int h, BC_Pixmap *pixmap)
{
	XDrawArc(top_level->display, 
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap, 
		top_level->gc, 
		x, 
		y, 
		(w - 1), 
		(h - 2), 
		0 * 64, 
		360 * 64);
}

void BC_WindowBase::draw_disc(int x, int y, int w, int h, BC_Pixmap *pixmap)
{
	XFillArc(top_level->display, 
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap, 
		top_level->gc, 
		x, 
		y, 
		(w - 1), 
		(h - 2), 
		0 * 64, 
		360 * 64);
}

void BC_WindowBase::clear_box(int x, int y, int w, int h, BC_Pixmap *pixmap)
{
	set_color(bg_color);
	XFillRectangle(top_level->display, 
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap, 
		top_level->gc, 
		x, 
		y, 
		w, 
		h);
}

void BC_WindowBase::draw_text(int x, 
	int y, 
	char *text, 
	int length, 
	BC_Pixmap *pixmap)
{
	if(length < 0) length = strlen(text);
	int boldface = top_level->current_font & BOLDFACE;
	int font = top_level->current_font & 0xff;

	switch(font)
	{
		case MEDIUM_7SEGMENT:
			for(int i = 0; i < length; i++)
			{
				VFrame *image;
				switch(text[i])
				{
					case '0':
						image = get_resources()->medium_7segment[0];
						break;
					case '1':
						image = get_resources()->medium_7segment[1];
						break;
					case '2':
						image = get_resources()->medium_7segment[2];
						break;
					case '3':
						image = get_resources()->medium_7segment[3];
						break;
					case '4':
						image = get_resources()->medium_7segment[4];
						break;
					case '5':
						image = get_resources()->medium_7segment[5];
						break;
					case '6':
						image = get_resources()->medium_7segment[6];
						break;
					case '7':
						image = get_resources()->medium_7segment[7];
						break;
					case '8':
						image = get_resources()->medium_7segment[8];
						break;
					case '9':
						image = get_resources()->medium_7segment[9];
						break;
					case ':':
						image = get_resources()->medium_7segment[10];
						break;
					case '.':
						image = get_resources()->medium_7segment[11];
						break;
					case 'a':
					case 'A':
						image = get_resources()->medium_7segment[12];
						break;
					case 'b':
					case 'B':
						image = get_resources()->medium_7segment[13];
						break;
					case 'c':
					case 'C':
						image = get_resources()->medium_7segment[14];
						break;
					case 'd':
					case 'D':
						image = get_resources()->medium_7segment[15];
						break;
					case 'e':
					case 'E':
						image = get_resources()->medium_7segment[16];
						break;
					case 'f':
					case 'F':
						image = get_resources()->medium_7segment[17];
						break;
					case ' ':
						image = get_resources()->medium_7segment[18];
						break;
					case '-':
						image = get_resources()->medium_7segment[19];
						break;
					default:
						image = get_resources()->medium_7segment[18];
						break;
				}

				draw_vframe(image, 
					x, 
					y - image->get_h());
				x += image->get_w();
			}
			break;

		default:
		{
// Set drawing color for dropshadow
			int color = get_color();
			if(boldface) set_color(BLACK);


			for(int k = (boldface ? 1 : 0); k >= 0; k--)
			{
				for(int i = 0, j = 0, x2 = x, y2 = y; 
					i <= length; 
					i++)
				{
					if(text[i] == '\n' || text[i] == 0)
					{
#ifdef HAVE_XFT
						if(get_resources()->use_xft && 
							top_level->get_xft_struct(top_level->current_font))
						{
							draw_xft_text(x,
								y,
								text,
								length,
								pixmap,
								x2,
								k,
								y2,
								j,
								i);
						}
						else
#endif
						if(get_resources()->use_fontset && top_level->get_curr_fontset())
						{
        					XmbDrawString(top_level->display, 
                				pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap, 
                				top_level->get_curr_fontset(),
                				top_level->gc, 
                				x2 + k, 
                				y2 + k, 
                				&text[j], 
                				i - j);
						}
						else
						{
//printf("BC_WindowBase::draw_text 3\n");
							XDrawString(top_level->display, 
								pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap, 
								top_level->gc, 
								x2 + k, 
								y2 + k, 
								&text[j], 
								i - j);
						}



						j = i + 1;
						y2 += get_text_height(MEDIUMFONT);
					}
				}
				if(boldface) set_color(color);
			}
			break;
		}
	}
}

void BC_WindowBase::draw_xft_text(int x, 
	int y, 
	char *text, 
	int length, 
	BC_Pixmap *pixmap,
	int x2,
	int k,
	int y2,
	int j,
	int i)
{
#ifdef HAVE_XFT
// printf("BC_WindowBase::draw_xft_text_synch 1 %d %p\n", 
// get_resources()->use_xft, 
// top_level->get_xft_struct(top_level->current_font));
	XRenderColor color;
	XftColor xft_color;
	color.red = (top_level->current_color & 0xff0000) >> 16;
	color.red |= color.red << 8;
	color.green = (top_level->current_color & 0xff00) >> 8;
	color.green |= color.green << 8;
	color.blue = (top_level->current_color & 0xff);
	color.blue |= color.blue << 8;
	color.alpha = 0xffff;

	XftColorAllocValue(top_level->display,
		top_level->vis,
		top_level->cmap,
		&color,
		&xft_color);

// printf("BC_WindowBase::draw_text 1 %u   %p %p %p %d %d %s %d\n",
// xft_color.pixel,
// pixmap ? pixmap->opaque_xft_draw : this->xft_drawable,
// &xft_color,
// top_level->get_xft_struct(top_level->current_font),
// x2 + k, 
// y2 + k,
// (FcChar8*)&text[j],
// i - j);
	XftDrawString8 (
		(XftDraw*)(pixmap ? pixmap->opaque_xft_draw : this->pixmap->opaque_xft_draw),
		&xft_color,
		top_level->get_xft_struct(top_level->current_font),
		x2 + k, 
		y2 + k,
		(FcChar8*)&text[j],
		i - j);
	XftColorFree(top_level->display,
	    top_level->vis,
	    top_level->cmap,
	    &xft_color);
#endif
}


void BC_WindowBase::draw_center_text(int x, int y, char *text, int length)
{
	if(length < 0) length = strlen(text);
	int w = get_text_width(current_font, text, length);
	x -= w / 2;
	draw_text(x, y, text, length);
}

void BC_WindowBase::draw_line(int x1, int y1, int x2, int y2, BC_Pixmap *pixmap)
{
	XDrawLine(top_level->display, 
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap, 
		top_level->gc, 
		x1, 
		y1, 
		x2, 
		y2);
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

	XDrawLines(top_level->display,
    	pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
    	top_level->gc,
    	points,
    	npoints,
    	CoordModeOrigin);

	delete [] points;
}


void BC_WindowBase::draw_rectangle(int x, int y, int w, int h)
{
	XDrawRectangle(top_level->display, 
		pixmap->opaque_pixmap, 
		top_level->gc, 
		x, 
		y, 
		w - 1, 
		h - 1);
}

void BC_WindowBase::draw_3d_border(int x, int y, int w, int h, 
	int light1, int light2, int shadow1, int shadow2)
{
	int lx, ly, ux, uy;

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
}

void BC_WindowBase::draw_3d_box(int x, 
	int y, 
	int w, 
	int h, 
	int light1, 
	int light2, 
	int middle, 
	int shadow1, 
	int shadow2,
	BC_Pixmap *pixmap)
{
	int lx, ly, ux, uy;

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
}

void BC_WindowBase::draw_colored_box(int x, int y, int w, int h, int down, int highlighted)
{
	if(!down)
	{
		if(highlighted)
			draw_3d_box(x, y, w, h, 
				top_level->get_resources()->button_light, 
				top_level->get_resources()->button_highlighted, 
				top_level->get_resources()->button_highlighted, 
				top_level->get_resources()->button_shadow,
				BLACK);
		else
			draw_3d_box(x, y, w, h, 
				top_level->get_resources()->button_light, 
				top_level->get_resources()->button_up, 
				top_level->get_resources()->button_up, 
				top_level->get_resources()->button_shadow,
				BLACK);
	}
	else
	{
// need highlighting for toggles
		if(highlighted)
			draw_3d_box(x, y, w, h, 
				top_level->get_resources()->button_shadow, 
				BLACK, 
				top_level->get_resources()->button_up, 
				top_level->get_resources()->button_up,
				top_level->get_resources()->button_light);
		else
			draw_3d_box(x, y, w, h, 
				top_level->get_resources()->button_shadow, 
				BLACK, 
				top_level->get_resources()->button_down, 
				top_level->get_resources()->button_down,
				top_level->get_resources()->button_light);
	}
}

void BC_WindowBase::draw_border(char *text, int x, int y, int w, int h)
{
	int left_indent = 20;
	int lx, ly, ux, uy;

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
	
	set_color(top_level->get_resources()->button_shadow);
	draw_line(x, y, x + left_indent - 5, y);
	draw_line(x, y, x, uy);
	draw_line(x + left_indent + 5 + get_text_width(MEDIUMFONT, text), y, ux, y);
	draw_line(x, y, x, uy);
	draw_line(ux, ly, ux, uy);
	draw_line(lx, uy, ux, uy);
	set_color(top_level->get_resources()->button_light);
	draw_line(lx, ly, x + left_indent - 5 - 1, ly);
	draw_line(lx, ly, lx, uy - 1);
	draw_line(x + left_indent + 5 + get_text_width(MEDIUMFONT, text), ly, ux - 1, ly);
	draw_line(lx, ly, lx, uy - 1);
	draw_line(x + w, y, x + w, y + h);
	draw_line(x, y + h, x + w, y + h);
}

void BC_WindowBase::draw_triangle_down_flat(int x, int y, int w, int h)
{
	int x1, y1, x2, y2, x3, y3;
	XPoint point[3];

	x1 = x; x2 = x + w / 2; x3 = x + w - 1;
	y1 = y; y2 = y + h - 1;

	point[0].x = x2; point[0].y = y2; point[1].x = x3;
	point[1].y = y1; point[2].x = x1; point[2].y = y1;

	XFillPolygon(top_level->display, 
		pixmap->opaque_pixmap, 
		top_level->gc, 
		(XPoint *)point, 
		3, 
		Nonconvex, 
		CoordModeOrigin);
}

void BC_WindowBase::draw_triangle_up(int x, int y, int w, int h, 
	int light1, int light2, int middle, int shadow1, int shadow2)
{
	int x1, y1, x2, y2, x3, y3;
	XPoint point[3];

	x1 = x; y1 = y; x2 = x + w / 2;
	y2 = y + h - 1; x3 = x + w - 1;

// middle
	point[0].x = x2; point[0].y = y1; point[1].x = x3;
	point[1].y = y2; point[2].x = x1; point[2].y = y2;

	set_color(middle);
	XFillPolygon(top_level->display, 
		pixmap->opaque_pixmap, 
		top_level->gc, 
		(XPoint *)point, 
		3, 
		Nonconvex, 
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
}

void BC_WindowBase::draw_triangle_down(int x, int y, int w, int h, 
	int light1, int light2, int middle, int shadow1, int shadow2)
{
	int x1, y1, x2, y2, x3, y3;
	XPoint point[3];

	x1 = x; x2 = x + w / 2; x3 = x + w - 1;
	y1 = y; y2 = y + h - 1;

	point[0].x = x2; point[0].y = y2; point[1].x = x3;
	point[1].y = y1; point[2].x = x1; point[2].y = y1;

	set_color(middle);
	XFillPolygon(top_level->display, 
		pixmap->opaque_pixmap, 
		top_level->gc, 
		(XPoint *)point, 
		3, 
		Nonconvex, 
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
}

void BC_WindowBase::draw_triangle_left(int x, int y, int w, int h, 
	int light1, int light2, int middle, int shadow1, int shadow2)
{
  	int x1, y1, x2, y2, x3, y3;
	XPoint point[3];

	// draw back arrow
  	y1 = y; x1 = x; y2 = y + h / 2;
  	x2 = x + w - 1; y3 = y + h - 1;

	point[0].x = x1; point[0].y = y2; point[1].x = x2; 
	point[1].y = y1; point[2].x = x2; point[2].y = y3;

	set_color(middle);
  	XFillPolygon(top_level->display, 
		pixmap->opaque_pixmap, 
		top_level->gc, 
		(XPoint *)point, 
		3, 
		Nonconvex, 
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
}

void BC_WindowBase::draw_triangle_right(int x, int y, int w, int h, 
	int light1, int light2, int middle, int shadow1, int shadow2)
{
  	int x1, y1, x2, y2, x3, y3;
	XPoint point[3];

	y1 = y; y2 = y + h / 2; y3 = y + h - 1; 
	x1 = x; x2 = x + w - 1;

	point[0].x = x1; point[0].y = y1; point[1].x = x2; 
	point[1].y = y2; point[2].x = x1; point[2].y = y3;

	set_color(middle);
  	XFillPolygon(top_level->display, 
		pixmap->opaque_pixmap, 
		top_level->gc, 
		(XPoint *)point, 
		3, 
		Nonconvex, 
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
}


void BC_WindowBase::draw_check(int x, int y)
{
	const int w = 15, h = 15;
	draw_line(x + 3, y + h / 2 + 0, x + 6, y + h / 2 + 2);
	draw_line(x + 3, y + h / 2 + 1, x + 6, y + h / 2 + 3);
	draw_line(x + 6, y + h / 2 + 2, x + w - 4, y + h / 2 - 3);
	draw_line(x + 3, y + h / 2 + 2, x + 6, y + h / 2 + 4);
	draw_line(x + 6, y + h / 2 + 2, x + w - 4, y + h / 2 - 3);
	draw_line(x + 6, y + h / 2 + 3, x + w - 4, y + h / 2 - 2);
	draw_line(x + 6, y + h / 2 + 4, x + w - 4, y + h / 2 - 1);
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
	XTranslateCoordinates(top_level->display, 
			parent_window->win, 
			win, 
			0, 
			0, 
			&origin_x, 
			&origin_y, 
			&tempwin);

	draw_tiles(parent_window->bg_pixmap, 
		origin_x,
		origin_y,
		x,
		y,
		w,
		h);
}

void BC_WindowBase::draw_top_background(BC_WindowBase *parent_window, 
	int x, 
	int y, 
	int w, 
	int h, 
	BC_Pixmap *pixmap)
{
	Window tempwin;
	int top_x, top_y;

	XTranslateCoordinates(top_level->display, 
			win, 
			parent_window->win, 
			x, 
			y, 
			&top_x, 
			&top_y, 
			&tempwin);

	XCopyArea(top_level->display, 
		parent_window->pixmap->opaque_pixmap, 
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap, 
		top_level->gc, 
		top_x, 
		top_y, 
		w, 
		h, 
		x, 
		y);
}

void BC_WindowBase::draw_background(int x, int y, int w, int h)
{
	if(bg_pixmap)
	{
		draw_tiles(bg_pixmap, 0, 0, x, y, w, h);
	}
	else
	{
		clear_box(x, y, w, h);
	}
}

void BC_WindowBase::draw_bitmap(BC_Bitmap *bitmap, 
	int dont_wait,
	int dest_x, 
	int dest_y,
	int dest_w,
	int dest_h,
	int src_x,
	int src_y,
	int src_w,
	int src_h,
	BC_Pixmap *pixmap)
{

// Hide cursor if video enabled
	update_video_cursor();

//printf("BC_WindowBase::draw_bitmap 1\n");
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
		bitmap->write_drawable(win, 
			top_level->gc, 
			src_x, 
			src_y, 
			src_w,
			src_h,
			dest_x, 
			dest_y, 
			dest_w, 
			dest_h, 
			dont_wait);
		top_level->flush();
	}
	else
	{
		bitmap->write_drawable(pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap, 
			top_level->gc, 
			dest_x, 
			dest_y, 
			src_x, 
			src_y, 
			dest_w, 
			dest_h, 
			dont_wait);
	}
//printf("BC_WindowBase::draw_bitmap 2\n");
}


void BC_WindowBase::draw_pixel(int x, int y, BC_Pixmap *pixmap)
{
	XDrawPoint(top_level->display, 
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap, 
		top_level->gc, 
		x, 
		y);
}


void BC_WindowBase::draw_pixmap(BC_Pixmap *pixmap, 
	int dest_x, 
	int dest_y,
	int dest_w,
	int dest_h,
	int src_x,
	int src_y,
	BC_Pixmap *dst)
{
	pixmap->write_drawable(dst ? dst->opaque_pixmap : this->pixmap->opaque_pixmap,
			dest_x, 
			dest_y,
			dest_w,
			dest_h,
			src_x,
			src_y);
}

void BC_WindowBase::draw_vframe(VFrame *frame, 
		int dest_x, 
		int dest_y, 
		int dest_w, 
		int dest_h,
		int src_x,
		int src_y,
		int src_w,
		int src_h,
		BC_Pixmap *pixmap)
{
	if(dest_w <= 0) dest_w = frame->get_w() - src_x;
	if(dest_h <= 0) dest_h = frame->get_h() - src_y;
	if(src_w <= 0) src_w = frame->get_w() - src_x;
	if(src_h <= 0) src_h = frame->get_h() - src_y;
	CLAMP(src_x, 0, frame->get_w() - 1);
	CLAMP(src_y, 0, frame->get_h() - 1);
	if(src_x + src_w > frame->get_w()) src_w = frame->get_w() - src_x;
	if(src_y + src_h > frame->get_h()) src_h = frame->get_h() - src_y;

	if(!temp_bitmap) temp_bitmap = new BC_Bitmap(this, 
		dest_w, 
		dest_h, 
		get_color_model(), 
		1);

	temp_bitmap->match_params(dest_w, 
		dest_h, 
		get_color_model(), 
		1);

	temp_bitmap->read_frame(frame, 
		src_x, 
		src_y, 
		src_w, 
		src_h,
		0, 
		0, 
		dest_w, 
		dest_h);

	draw_bitmap(temp_bitmap, 
		0, 
		dest_x, 
		dest_y,
		dest_w,
		dest_h,
		0,
		0,
		-1,
		-1,
		pixmap);
}

void BC_WindowBase::draw_tooltip()
{
	if(tooltip_popup)
	{
		int w = tooltip_popup->get_w(), h = tooltip_popup->get_h();
		tooltip_popup->set_color(get_resources()->tooltip_bg_color);
		tooltip_popup->draw_box(0, 0, w, h);
		tooltip_popup->set_color(BLACK);
		tooltip_popup->draw_rectangle(0, 0, w, h);
		tooltip_popup->set_font(MEDIUMFONT);
		tooltip_popup->draw_text(TOOLTIP_MARGIN, 
			get_text_ascent(MEDIUMFONT) + TOOLTIP_MARGIN, 
			tooltip_text);
	}
}

void BC_WindowBase::slide_left(int distance)
{
	if(distance < w)
	{
		XCopyArea(top_level->display, 
			pixmap->opaque_pixmap, 
			pixmap->opaque_pixmap, 
			top_level->gc, 
			distance, 
			0, 
			w - distance, 
			h, 
			0, 
			0);
	}
}

void BC_WindowBase::slide_right(int distance)
{
	if(distance < w)
	{
		XCopyArea(top_level->display, 
			pixmap->opaque_pixmap, 
			pixmap->opaque_pixmap, 
			top_level->gc, 
			0, 
			0, 
			w - distance, 
			h, 
			distance, 
			0);
	}
}

void BC_WindowBase::slide_up(int distance)
{
	if(distance < h)
	{
		XCopyArea(top_level->display, 
			pixmap->opaque_pixmap, 
			pixmap->opaque_pixmap, 
			top_level->gc, 
			0, 
			distance, 
			w, 
			h - distance, 
			0, 
			0);
		set_color(bg_color);
		XFillRectangle(top_level->display, 
			pixmap->opaque_pixmap, 
			top_level->gc, 
			0, 
			h - distance, 
			w, 
			distance);
	}
}

void BC_WindowBase::slide_down(int distance)
{
	if(distance < h)
	{
		XCopyArea(top_level->display, 
			pixmap->opaque_pixmap, 
			pixmap->opaque_pixmap, 
			top_level->gc, 
			0, 
			0, 
			w, 
			h - distance, 
			0, 
			distance);
		set_color(bg_color);
		XFillRectangle(top_level->display, 
			pixmap->opaque_pixmap, 
			top_level->gc, 
			0, 
			0, 
			w, 
			distance);
	}
}

// 3 segments in separate pixmaps.  Obsolete.
void BC_WindowBase::draw_3segment(int x, 
	int y, 
	int w, 
	int h, 
	BC_Pixmap *left_image,
	BC_Pixmap *mid_image,
	BC_Pixmap *right_image,
	BC_Pixmap *pixmap)
{
	if(w <= 0 || h <= 0) return;
	int left_boundary = left_image->get_w_fixed();
	int right_boundary = w - right_image->get_w_fixed();
	for(int i = 0; i < w; )
	{
		BC_Pixmap *image;

		if(i < left_boundary)
			image = left_image;
		else
		if(i < right_boundary)
			image = mid_image;
		else
			image = right_image;

		int output_w = image->get_w_fixed();

		if(i < left_boundary)
		{
			if(i + output_w > left_boundary) output_w = left_boundary - i;
		}
		else
		if(i < right_boundary)
		{
			if(i + output_w > right_boundary) output_w = right_boundary - i;
		}
		else
			if(i + output_w > w) output_w = w - i;

		image->write_drawable(pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap, 
				x + i, 
				y,
				output_w,
				h,
				0,
				0);

		i += output_w;
	}
}
// 3 segments in separate vframes.  Obsolete.
void BC_WindowBase::draw_3segment(int x, 
	int y, 
	int w, 
	int h, 
	VFrame *left_image,
	VFrame *mid_image,
	VFrame *right_image,
	BC_Pixmap *pixmap)
{
	if(w <= 0 || h <= 0) return;
	int left_boundary = left_image->get_w_fixed();
	int right_boundary = w - right_image->get_w_fixed();


	for(int i = 0; i < w; )
	{
		VFrame *image;

		if(i < left_boundary)
			image = left_image;
		else
		if(i < right_boundary)
			image = mid_image;
		else
			image = right_image;
		
		int output_w = image->get_w_fixed();

		if(i < left_boundary)
		{
			if(i + output_w > left_boundary) output_w = left_boundary - i;
		}
		else
		if(i < right_boundary)
		{
			if(i + output_w > right_boundary) output_w = right_boundary - i;
		}
		else
			if(i + output_w > w) output_w = w - i;

		if(image)
			draw_vframe(image, 
					x + i, 
					y,
					output_w,
					h,
					0,
					0,
					0,
					0,
					pixmap);

		if(output_w == 0) break;
		i += output_w;
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


void BC_WindowBase::draw_3segmenth(int x, 
		int y, 
		int w, 
		VFrame *image,
		BC_Pixmap *pixmap)
{
	draw_3segmenth(x, 
		y, 
		w, 
		x,
		w,
		image,
		pixmap);
}

void BC_WindowBase::draw_3segmenth(int x, 
		int y, 
		int w, 
		int total_x,
		int total_w,
		VFrame *image,
		BC_Pixmap *pixmap)
{
	if(total_w <= 0 || w <= 0 || h <= 0) return;
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

//printf("BC_WindowBase::draw_3segment 1 left_out_x=%d left_out_w=%d center_out_x=%d center_out_w=%d right_out_x=%d right_out_w=%d\n", 
//	left_out_x, left_out_w, center_out_x, center_out_w, right_out_x, right_out_w);

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

	if(!temp_bitmap) temp_bitmap = new BC_Bitmap(top_level, 
		image->get_w(), 
		image->get_h(), 
		get_color_model(), 
		0);
	temp_bitmap->match_params(image->get_w(), 
		image->get_h(), 
		get_color_model(), 
		0);
	temp_bitmap->read_frame(image, 
		0, 
		0, 
		image->get_w(), 
		image->get_h());


//printf("BC_WindowBase::draw_3segment 2 left_out_x=%d left_out_w=%d center_out_x=%d center_out_w=%d right_out_x=%d right_out_w=%d\n", 
//	left_out_x, left_out_w, center_out_x, center_out_w, right_out_x, right_out_w);
	if(left_out_w > 0)
	{
		draw_bitmap(temp_bitmap, 
			0, 
			left_out_x, 
			y,
			left_out_w,
			image->get_h(),
			left_in_x,
			0,
			-1,   // src width and height are meaningless in video_off mode
			-1,
			pixmap);
	}

	if(right_out_w > 0)
	{
		draw_bitmap(temp_bitmap, 
			0, 
			right_out_x, 
			y,
			right_out_w,
			image->get_h(),
			right_in_x,
			0,
			-1,   // src width and height are meaningless in video_off mode
			-1,
			pixmap);
	}

	for(int pixel = center_out_x; 
		pixel < center_out_x + center_out_w; 
		pixel += half_image)
	{
		int fragment_w = half_image;
		if(fragment_w + pixel > center_out_x + center_out_w)
			fragment_w = (center_out_x + center_out_w) - pixel;

//printf("BC_WindowBase::draw_3segment 2 pixel=%d fragment_w=%d\n", pixel, fragment_w);
		draw_bitmap(temp_bitmap, 
			0, 
			pixel, 
			y,
			fragment_w,
			image->get_h(),
			third_image,
			0,
			-1,   // src width and height are meaningless in video_off mode
			-1,
			pixmap);
	}

}







void BC_WindowBase::draw_3segmenth(int x, 
		int y, 
		int w, 
		int total_x,
		int total_w,
		BC_Pixmap *src,
		BC_Pixmap *dst)
{
	if(w <= 0 || total_w <= 0) return;
	if(!src) printf("BC_WindowBase::draw_3segmenth src=0\n");
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

//printf("BC_WindowBase::draw_3segment 1 left_out_x=%d left_out_w=%d center_out_x=%d center_out_w=%d right_out_x=%d right_out_w=%d\n", 
//	left_out_x, left_out_w, center_out_x, center_out_w, right_out_x, right_out_w);

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


//printf("BC_WindowBase::draw_3segment 2 left_out_x=%d left_out_w=%d center_out_x=%d center_out_w=%d right_out_x=%d right_out_w=%d\n", 
//	left_out_x, left_out_w, center_out_x, center_out_w, right_out_x, right_out_w);
	if(left_out_w > 0)
	{
		draw_pixmap(src, 
			left_out_x, 
			y,
			left_out_w,
			src->get_h(),
			left_in_x,
			0,
			dst);
	}

	if(right_out_w > 0)
	{
		draw_pixmap(src, 
			right_out_x, 
			y,
			right_out_w,
			src->get_h(),
			right_in_x,
			0,
			dst);
	}

	for(int pixel = center_out_x; 
		pixel < center_out_x + center_out_w; 
		pixel += half_src)
	{
		int fragment_w = half_src;
		if(fragment_w + pixel > center_out_x + center_out_w)
			fragment_w = (center_out_x + center_out_w) - pixel;

//printf("BC_WindowBase::draw_3segment 2 pixel=%d fragment_w=%d\n", pixel, fragment_w);
		draw_pixmap(src, 
			pixel, 
			y,
			fragment_w,
			src->get_h(),
			quarter_src,
			0,
			dst);
	}

}


void BC_WindowBase::draw_3segmenth(int x, 
		int y, 
		int w,
		BC_Pixmap *src,
		BC_Pixmap *dst)
{
	if(w <= 0) return;
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

//printf("BC_WindowBase::draw_3segment 1 left_out_x=%d left_out_w=%d center_out_x=%d center_out_w=%d right_out_x=%d right_out_w=%d\n", 
//	left_out_x, left_out_w, center_out_x, center_out_w, right_out_x, right_out_w);

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

//printf("BC_WindowBase::draw_3segment 2 left_out_x=%d left_out_w=%d center_out_x=%d center_out_w=%d right_out_x=%d right_out_w=%d\n", 
//	left_out_x, left_out_w, center_out_x, center_out_w, right_out_x, right_out_w);
	if(left_out_w > 0)
	{
		draw_pixmap(src, 
			left_out_x, 
			y,
			left_out_w,
			src->get_h(),
			left_in_x,
			0,
			dst);
	}

	if(right_out_w > 0)
	{
		draw_pixmap(src, 
			right_out_x, 
			y,
			right_out_w,
			src->get_h(),
			right_in_x,
			0,
			dst);
	}

	for(int pixel = left_out_x + left_out_w; 
		pixel < right_out_x; 
		pixel += third_image)
	{
		int fragment_w = third_image;
		if(fragment_w + pixel > right_out_x)
			fragment_w = right_out_x - pixel;

//printf("BC_WindowBase::draw_3segment 2 pixel=%d fragment_w=%d\n", pixel, fragment_w);
		draw_pixmap(src, 
			pixel, 
			y,
			fragment_w,
			src->get_h(),
			third_image,
			0,
			dst);
	}

}







void BC_WindowBase::draw_3segmentv(int x, 
		int y, 
		int h,
		VFrame *src,
		BC_Pixmap *dst)
{
	if(h <= 0) return;
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


	if(!temp_bitmap) temp_bitmap = new BC_Bitmap(top_level, 
		src->get_w(), 
		src->get_h(), 
		get_color_model(), 
		0);
	temp_bitmap->match_params(src->get_w(), 
		src->get_h(), 
		get_color_model(), 
		0);
	temp_bitmap->read_frame(src, 
		0, 
		0, 
		src->get_w(), 
		src->get_h());


	if(left_out_h > 0)
	{
		draw_bitmap(temp_bitmap, 
			0,
			x,
			left_out_y, 
			src->get_w(),
			left_out_h,
			0,
			left_in_y,
			-1,
			-1,
			dst);
	}

	if(right_out_h > 0)
	{
		draw_bitmap(temp_bitmap, 
			0,
			x,
			right_out_y, 
			src->get_w(),
			right_out_h,
			0,
			right_in_y,
			-1,
			-1,
			dst);
	}

	for(int pixel = left_out_y + left_out_h; 
		pixel < right_out_y; 
		pixel += third_image)
	{
		int fragment_h = third_image;
		if(fragment_h + pixel > right_out_y)
			fragment_h = right_out_y - pixel;

//printf("BC_WindowBase::draw_3segment 2 pixel=%d fragment_w=%d\n", pixel, fragment_w);
		draw_bitmap(temp_bitmap, 
			0,
			x,
			pixel, 
			src->get_w(),
			fragment_h,
			0,
			third_image,
			-1,
			-1,
			dst);
	}
}

void BC_WindowBase::draw_3segmentv(int x, 
		int y, 
		int h,
		BC_Pixmap *src,
		BC_Pixmap *dst)
{
	if(h <= 0) return;
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
	{
		draw_pixmap(src, 
			x,
			left_out_y, 
			src->get_w(),
			left_out_h,
			0,
			left_in_y,
			dst);
	}

	if(right_out_h > 0)
	{
		draw_pixmap(src, 
			x,
			right_out_y, 
			src->get_w(),
			right_out_h,
			0,
			right_in_y,
			dst);
	}

	for(int pixel = left_out_y + left_out_h; 
		pixel < right_out_y; 
		pixel += third_image)
	{
		int fragment_h = third_image;
		if(fragment_h + pixel > right_out_y)
			fragment_h = right_out_y - pixel;

//printf("BC_WindowBase::draw_3segment 2 pixel=%d fragment_w=%d\n", pixel, fragment_w);
		draw_pixmap(src, 
			x,
			pixel, 
			src->get_w(),
			fragment_h,
			0,
			third_image,
			dst);
	}
}


void BC_WindowBase::draw_9segment(int x, 
		int y, 
		int w,
		int h,
		BC_Pixmap *src,
		BC_Pixmap *dst)
{
	if(w <= 0 || h <= 0) return;

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
	draw_pixmap(src,
		x + out_x1, 
		y + out_y1,
		out_x2 - out_x1,
		out_y2 - out_y1,
		in_x1, 
		in_y1, 
		dst);


// Segment 2 * n
	for(int i = out_x2; i < out_x3; i += in_x3 - in_x2)
	{
		if(out_x3 - i > 0)
		{
			int w = MIN(in_x3 - in_x2, out_x3 - i);
			draw_pixmap(src,
				x + i, 
				y + out_y1,
				w,
				out_y2 - out_y1,
				in_x2, 
				in_y1, 
				dst);
		}
	}





// Segment 3
	draw_pixmap(src,
		x + out_x3, 
		y + out_y1,
		out_x4 - out_x3,
		out_y2 - out_y1,
		in_x3, 
		in_y1, 
		dst);



// Segment 4 * n
	for(int i = out_y2; i < out_y3; i += in_y3 - in_y2)
	{
		if(out_y3 - i > 0)
		{
			int h = MIN(in_y3 - in_y2, out_y3 - i);
			draw_pixmap(src,
				x + out_x1, 
				y + i,
				out_x2 - out_x1,
				h,
				in_x1, 
				in_y2, 
				dst);
		}
	}


// Segment 5 * n * n
	for(int i = out_y2; i < out_y3; i += in_y3 - in_y2 /* in_y_third */)
	{
		if(out_y3 - i > 0)
		{
			int h = MIN(in_y3 - in_y2 /* in_y_third */, out_y3 - i);


			for(int j = out_x2; j < out_x3; j += in_x3 - in_x2 /* in_x_third */)
			{
				int w = MIN(in_x3 - in_x2 /* in_x_third */, out_x3 - j);
				if(out_x3 - j > 0)
					draw_pixmap(src,
						x + j, 
						y + i,
						w,
						h,
						in_x2, 
						in_y2, 
						dst);
			}
		}
	}

// Segment 6 * n
	for(int i = out_y2; i < out_y3; i += in_y3 - in_y2)
	{
		if(out_y3 - i > 0)
		{
			int h = MIN(in_y3 - in_y2, out_y3 - i);
			draw_pixmap(src,
				x + out_x3, 
				y + i,
				out_x4 - out_x3,
				h,
				in_x3, 
				in_y2, 
				dst);
		}
	}




// Segment 7
	draw_pixmap(src,
		x + out_x1, 
		y + out_y3,
		out_x2 - out_x1,
		out_y4 - out_y3,
		in_x1, 
		in_y3, 
		dst);


// Segment 8 * n
	for(int i = out_x2; i < out_x3; i += in_x3 - in_x2)
	{
		if(out_x3 - i > 0)
		{
			int w = MIN(in_x3 - in_y2, out_x3 - i);
			draw_pixmap(src,
				x + i, 
				y + out_y3,
				w,
				out_y4 - out_y3,
				in_x2, 
				in_y3, 
				dst);
		}
	}



// Segment 9
	draw_pixmap(src,
		x + out_x3, 
		y + out_y3,
		out_x4 - out_x3,
		out_y4 - out_y3,
		in_x3, 
		in_y3, 
		dst);
}


void BC_WindowBase::draw_9segment(int x, 
		int y, 
		int w,
		int h,
		VFrame *src,
		BC_Pixmap *dst)
{
	if(w <= 0 || h <= 0) return;

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

//printf("PFCFrame::draw_9segment 1 %d %d %d %d\n", out_x1, out_x2, out_x3, out_x4);
//printf("PFCFrame::draw_9segment 2 %d %d %d %d\n", in_x1, in_x2, in_x3, in_x4);
//printf("PFCFrame::draw_9segment 2 %d %d %d %d\n", in_y1, in_y2, in_y3, in_y4);

	if(!temp_bitmap) temp_bitmap = new BC_Bitmap(top_level, 
		src->get_w(), 
		src->get_h(), 
		get_color_model(), 
		0);
	temp_bitmap->match_params(src->get_w(), 
		src->get_h(), 
		get_color_model(), 
		0);
	temp_bitmap->read_frame(src, 
		0, 
		0, 
		src->get_w(), 
		src->get_h());

// Segment 1
	draw_bitmap(temp_bitmap,
		0,
		x + out_x1, 
		y + out_y1,
		out_x2 - out_x1,
		out_y2 - out_y1,
		in_x1, 
		in_y1, 
		in_x2 - in_x1,
		in_y2 - in_y1, 
		dst);


// Segment 2 * n
	for(int i = out_x2; i < out_x3; i += in_x3 - in_x2)
	{
		if(out_x3 - i > 0)
		{
			int w = MIN(in_x3 - in_x2, out_x3 - i);
			draw_bitmap(temp_bitmap,
				0,
				x + i, 
				y + out_y1,
				w,
				out_y2 - out_y1,
				in_x2, 
				in_y1, 
				w,
				in_y2 - in_y1, 
				dst);
		}
	}





// Segment 3
	draw_bitmap(temp_bitmap,
		0,
		x + out_x3, 
		y + out_y1,
		out_x4 - out_x3,
		out_y2 - out_y1,
		in_x3, 
		in_y1, 
		in_x4 - in_x3,
		in_y2 - in_y1, 
		dst);



// Segment 4 * n
	for(int i = out_y2; i < out_y3; i += in_y3 - in_y2)
	{
		if(out_y3 - i > 0)
		{
			int h = MIN(in_y3 - in_y2, out_y3 - i);
			draw_bitmap(temp_bitmap,
				0,
				x + out_x1, 
				y + i,
				out_x2 - out_x1,
				h,
				in_x1, 
				in_y2, 
				in_x2 - in_x1,
				h, 
				dst);
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
						0,
						x + j, 
						y + i,
						w,
						h,
						in_x2, 
						in_y2, 
						w,
						h, 
						dst);
			}
		}
	}

// Segment 6 * n
	for(int i = out_y2; i < out_y3; i += in_y_third)
	{
		if(out_y3 - i > 0)
		{
			int h = MIN(in_y_third, out_y3 - i);
			draw_bitmap(temp_bitmap,
				0,
				x + out_x3, 
				y + i,
				out_x4 - out_x3,
				h,
				in_x3, 
				in_y2, 
				in_x4 - in_x3,
				h, 
				dst);
		}
	}




// Segment 7
	draw_bitmap(temp_bitmap,
		0,
		x + out_x1, 
		y + out_y3,
		out_x2 - out_x1,
		out_y4 - out_y3,
		in_x1, 
		in_y3, 
		in_x2 - in_x1,
		in_y4 - in_y3, 
		dst);


// Segment 8 * n
	for(int i = out_x2; i < out_x3; i += in_x_third)
	{
		if(out_x3 - i > 0)
		{
			int w = MIN(in_x_third, out_x3 - i);
			draw_bitmap(temp_bitmap,
				0,
				x + i, 
				y + out_y3,
				w,
				out_y4 - out_y3,
				in_x2, 
				in_y3, 
				w,
				in_y4 - in_y3, 
				dst);
		}
	}



// Segment 9
	draw_bitmap(temp_bitmap,
		0,
		x + out_x3, 
		y + out_y3,
		out_x4 - out_x3,
		out_y4 - out_y3,
		in_x3, 
		in_y3, 
		in_x4 - in_x3,
		in_y4 - in_y3, 
		dst);
}










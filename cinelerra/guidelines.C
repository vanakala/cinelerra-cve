
/*
 * CINELERRA
 * Copyright (C) 2018 Einar Rünkaru <einarrunkaru@gmail dot com>
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

#include "edl.h"
#include "canvas.h"
#include "colors.h"
#include "edlsession.h"
#include "guidelines.h"
#include "mwindow.h"

#include <string.h>

#define GUIDELINE_UNIT 128

extern MWindow *mwindow_global;

GuideFrame::GuideFrame(ptstime start, ptstime end)
 : ListItem<GuideFrame>()
{
	this->start = start;
	this->end = end;
	period = 0;
	allocated = 0;
	data = 0;
	dataend = 0;
	is_enabled = 0;
}

GuideFrame::~GuideFrame()
{
	delete [] data;
}

void GuideFrame::check_alloc(int num)
{
	uint16_t *new_alloc;

	if(num > 0 && num + (dataend - data) > allocated)
	{
		int len = dataend - data + num;

		if(allocated + GUIDELINE_UNIT > len)
			len = allocated + GUIDELINE_UNIT;
		new_alloc = new uint16_t[len];
		if(data)
			memcpy(new_alloc, data, allocated);
		dataend = new_alloc + (dataend - data);
		delete [] data;
		data = new_alloc;
		allocated = len;
	}
	is_enabled = 1;
}

void GuideFrame::add_line(int x1, int y1, int x2, int y2)
{
	check_alloc(5);
	*dataend++ = GUIDELINE_LINE;
	*dataend++ = x1;
	*dataend++ = y1;
	*dataend++ = x2;
	*dataend++ = y2;
}

void GuideFrame::add_rectangle(int x1, int y1, int w, int h)
{
	check_alloc(5);
	*dataend++ = GUIDELINE_RECTANGLE;
	*dataend++ = x1;
	*dataend++ = y1;
	*dataend++ = w;
	*dataend++ = h;
}

void GuideFrame::add_box(int x1, int y1, int w, int h)
{
	check_alloc(5);
	*dataend++ = GUIDELINE_BOX;
	*dataend++ = x1;
	*dataend++ = y1;
	*dataend++ = w;
	*dataend++ = h;
}

void GuideFrame::add_disc(int x, int y, int w, int h)
{
	check_alloc(5);
	*dataend++ = GUIDELINE_DISC;
	*dataend++ = x;
	*dataend++ = y;
	*dataend++ = w;
	*dataend++ = h;
}

void GuideFrame::add_circle(int x, int y, int w, int h)
{
	check_alloc(5);
	*dataend++ = GUIDELINE_CIRCLE;
	*dataend++ = x;
	*dataend++ = y;
	*dataend++ = w;
	*dataend++ = h;
}

void GuideFrame::add_pixel(int x, int y)
{
	check_alloc(3);
	*dataend++ = GUIDELINE_PIXEL;
	*dataend++ = x;
	*dataend++ = y;
}

void GuideFrame::clear()
{
	is_enabled = 0;
	dataend = data;
}

int GuideFrame::set_enabled(int value)
{
	int rv = is_enabled;

	is_enabled = value;
	return rv;
}

void GuideFrame::shift(ptstime difference)
{
	start += difference;
	end += difference;
}

void GuideFrame::set_position(ptstime new_start, ptstime new_end)
{
	start = new_start;
	end = new_end;
}

void GuideFrame::draw(Canvas *canvas, EDL *edl, ptstime pts)
{
	uint16_t *dp;
	double x1, x2, y1, y2;

	if(is_enabled && data && start <= pts && pts < end)
	{
		for(dp = data; dp < dataend;)
		{
			x1 = dp[1];
			y1 = dp[2];
			canvas->output_to_canvas(edl, x1, y1);
			if(*dp != GUIDELINE_PIXEL)
			{
				if(*dp == GUIDELINE_LINE)
				{
					x2 = dp[3];
					y2 = dp[4];
					canvas->output_to_canvas(edl, x2, y2);
				}
				else
				{
					x2 = dp[1] + dp[3];
					y2 = dp[2] + dp[4];
					canvas->output_to_canvas(edl, x2, y2);
					x2 -= x1;
					y2 -= y1;
				}
			}
			canvas->get_canvas()->set_color(WHITE);

			switch(*dp++)
			{
			case GUIDELINE_LINE:
				canvas->get_canvas()->draw_line(
					round(x1), round(y1),
					round(x2), round(y2));
				dp += 4;
				break;
			case GUIDELINE_RECTANGLE:
				canvas->get_canvas()->draw_rectangle(
					round(x1), round(y1),
					round(x2), round(y2));
				dp += 4;
				break;
			case GUIDELINE_BOX:
				canvas->get_canvas()->draw_box(
					round(x1), round(y1),
					round(x2), round(y2));
				dp += 4;
				break;
			case GUIDELINE_DISC:
				canvas->get_canvas()->draw_disc(
					round(x1), round(y1),
					round(x2), round(y2));
				dp += 4;
				break;
			case GUIDELINE_CIRCLE:
				canvas->get_canvas()->draw_circle(
					round(x1), round(y1),
					round(x2), round(y2));
				dp += 4;
				break;
			case GUIDELINE_PIXEL:
				canvas->get_canvas()->draw_pixel(
					round(x1), round(y1));
				dp += 2;
				break;
			default:
				break;
			}
		}
	}
}

void GuideFrame::dump(int indent)
{
	uint16_t *p;
	int n;
	const char *s;

	printf("%*sGuideframe %p dump:\n", indent, "", this);
	indent += 2;
	printf("%*speriod %d is_enabled %d %.2f .. %.2f\n", indent, "",
		period, is_enabled, start, end);
	printf("%*sallocated %d data %p\n", indent, "",
		allocated, data);

	if(data)
	{
		for(p = data; p < dataend;)
		{
			switch(*p)
			{
			case GUIDELINE_LINE:
				s = "Line: ";
				n = 4;
				break;
			case GUIDELINE_RECTANGLE:
				s = "Rectangle: ";
				n = 4;
				break;
			case GUIDELINE_BOX:
				s = "Box: ";
				n = 4;
				break;
			case GUIDELINE_DISC:
				s = "Disc: ";
				n = 4;
				break;
			case GUIDELINE_CIRCLE:
				s = "Circle: ";
				n = 4;
				break;
			case GUIDELINE_PIXEL:
				s = "Pixel: ";
				n = 2;
				break;
			default:
				s = 0;
				n = 0;
				break;
			}
			if(s)
				printf("%*s%s", indent, "", s);
			else
				printf("%*sUnknown (%hd)", indent, "", *p);
			p++;
			for(int i = 0; i < n; i++)
				printf(" %hd", *p++);
			putchar('\n');
		}
	}
}


GuideLines::GuideLines()
 : List<GuideFrame>()
{
}

GuideLines::~GuideLines()
{
	delete_guideframes();
}

void GuideLines::delete_guideframes()
{
	while(last)
		delete last;
}

GuideFrame *GuideLines::append_frame(ptstime start, ptstime end)
{
	return append(new GuideFrame(start, end));
}

void GuideLines::draw(Canvas *canvas, EDL *edl, ptstime pts)
{
	GuideFrame *current;

	canvas->get_canvas()->set_inverse();
	for(current = first; current; current = current->next)
		current->draw(canvas, edl, pts);
	canvas->get_canvas()->set_opaque();
}

void GuideLines::dump(int indent)
{
	GuideFrame *current;

	printf("%*sGuidelines %p dump:\n", indent, "", this);
	for(current = first; current; current = current->next)
	{
		current->dump(indent + 2);
	}
}


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
#include "vframe.h"

#include <string.h>

#define GUIDELINE_UNIT 128
#define GUIDELINE_PERIOD 250

GuideFrame::GuideFrame(ptstime start, ptstime end, Canvas *canvas)
 : ListItem<GuideFrame>()
{
	this->start = start;
	this->end = end;
	this->canvas = canvas;
	period = 0;
	allocated = 0;
	data = 0;
	dataend = 0;
	is_enabled = 0;
	color = WHITE;
	vframe = 0;
	width = 0;
	height = 0;
}

GuideFrame::~GuideFrame()
{
	delete [] data;
	delete vframe;
}

void GuideFrame::check_alloc(int num)
{
	uint16_t *new_alloc;

	if(num > 0 && num + (dataend - data) > allocated)
	{
		canvas->lock_canvas("GuideFrame::check_alloc");
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
		canvas->unlock_canvas();
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

void GuideFrame::add_vframe()
{
	check_alloc(1);
	*dataend++ = GUIDELINE_VFRAME;
}

void GuideFrame::clear()
{
	period_count = 0;
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

void GuideFrame::set_color(int color)
{
	this->color = color;
}

void GuideFrame::set_dimensions(int w, int h)
{
	width = w;
	height = h;
}

VFrame *GuideFrame::get_vframe(int w, int h)
{
	canvas->lock_canvas("GuideFrame::get_vframe");
	if(vframe && (vframe->get_w() != w ||
		vframe->get_h() != h))
	{
		delete vframe;
		vframe = 0;
	}
	if(!vframe)
	{
		vframe = new VFrame(0, w, h, BC_A8);
		vframe->clear_frame();
	}
	canvas->unlock_canvas();
	return vframe;
}

void GuideFrame::repeat_event(Canvas *canvas)
{
	if(period)
	{
		if(period_count == 1)
			draw(canvas, edl, pts);
		else
		if(period_count > 0)
			period_count--;
	}
}

int GuideFrame::draw(Canvas *canvas, EDL *edl, ptstime pts)
{
	uint16_t *dp;
	double x1, x2, y1, y2;
	double hbase, vbase;

	if(is_enabled && data && start <= pts && pts < end)
	{
		if(width && height)
		{
			hbase = (edlsession->output_w - width) / 2;
			vbase = (edlsession->output_h - height) / 2;
		}
		else
			hbase = vbase = 0;

		for(dp = data; dp < dataend;)
		{
			x1 = dp[1] + hbase;
			y1 = dp[2] + vbase;

			canvas->output_to_canvas(edl, x1, y1);
			if(*dp != GUIDELINE_PIXEL)
			{
				if(*dp == GUIDELINE_LINE)
				{
					x2 = dp[3] + hbase;
					y2 = dp[4] + vbase;
					canvas->output_to_canvas(edl, x2, y2);
				}
				else
				{
					x2 = (dp[1] + dp[3]) + hbase;
					y2 = (dp[2] + dp[4]) + vbase;
					canvas->output_to_canvas(edl, x2, y2);
					x2 -= x1;
					y2 -= y1;
				}
			}
			canvas->get_canvas()->set_color(color);

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
			case GUIDELINE_VFRAME:
				if(vframe)
				{
					double ix1, ix2, iy1, iy2;

					canvas->get_transfers(edl,
						ix1, iy1, ix2, iy2,
						x1, y1, x2, y2);
					vframe->set_pixel_aspect(canvas->sample_aspect_ratio());
					canvas->get_canvas()->draw_vframe(vframe,
						round(x1), round(y1),
						round(x2 - x1), round(y2 - y1),
						round(ix1), round(iy1),
						round(ix2 - ix1), round(iy2 - iy1),
						0);
				}
				break;
			default:
				break;
			}
		}
		if(period)
		{
			period_count = period;
			this->edl = edl;
			this->pts = pts;
		}
		return 1;
	}
	return 0;
}

void GuideFrame::set_repeater_period(int period)
{
	this->period = period;
	period_count = period;
}

int GuideFrame::has_repeater_period()
{
	if(period)
		return 1;
	return 0;
}

size_t GuideFrame::get_size()
{
	size_t size = sizeof(*this);

	return size += allocated * sizeof(uint16_t);
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
	printf("%*sallocated %d data %p color %#x\n", indent, "",
		allocated, data, color);

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
			case GUIDELINE_VFRAME:
				s = "VFrame. ";
				n = 0;
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
	canvas = 0;
	repeater_window = 0;
}

GuideLines::~GuideLines()
{
	delete_guideframes();
}

void GuideLines::set_canvas(Canvas *canvas)
{
	this->canvas = canvas;
}

void GuideLines::delete_guideframes()
{
	stop_repeater();
	canvas->lock_canvas("GuideLines::delete_guideframes");
	while(last)
		delete last;
	canvas->unlock_canvas();
}

GuideFrame *GuideLines::append_frame(ptstime start, ptstime end)
{
	return append(new GuideFrame(start, end, canvas));
}

void GuideLines::draw(EDL *edl, ptstime pts)
{
	GuideFrame *current;
	int have_repeater = 0;

	canvas->get_canvas()->set_inverse();
	for(current = first; current; current = current->next)
	{
		if(current->draw(canvas, edl, pts))
			have_repeater |= current->has_repeater_period();
	}
	canvas->get_canvas()->set_opaque();

	if(have_repeater)
		activate_repeater();
	else
		stop_repeater();
}

void GuideLines::activate_repeater()
{
	BC_WindowBase *act_win = canvas->get_canvas();

	if(act_win != repeater_window)
		stop_repeater();

	if(!repeater_window)
	{
		act_win->set_repeat(GUIDELINE_PERIOD);
		repeater_window = act_win;
	}
}

void GuideLines::stop_repeater()
{
	if(repeater_window)
	{
		repeater_window->unset_repeat(GUIDELINE_PERIOD);
		repeater_window = 0;
	}
}

void GuideLines::repeat_event(int duration)
{
	GuideFrame *current;

	if(duration == GUIDELINE_PERIOD && canvas)
	{
		canvas->lock_canvas("GuideLines::repeat_event");
		canvas->get_canvas()->set_inverse();
		for(current = first; current; current = current->next)
			current->repeat_event(canvas);
		canvas->get_canvas()->set_opaque();
		canvas->get_canvas()->flash();
		canvas->unlock_canvas();
	}
}

size_t GuideLines::get_size()
{
	size_t size = sizeof(*this);

	for(GuideFrame *current = first; current; current = current->next)
		size += current->get_size();
	return size;
}

void GuideLines::dump(int indent)
{
	GuideFrame *current;

	printf("%*sGuidelines %p dump:\n", indent, "", this);
	indent += 2;
	printf("%*scanvas %p repeater_window %p\n", indent, "",
		canvas, repeater_window);
	for(current = first; current; current = current->next)
	{
		current->dump(indent + 2);
	}
}

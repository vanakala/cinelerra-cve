// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2018 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "bcsignals.h"
#include "edl.h"
#include "canvas.h"
#include "colors.h"
#include "edlsession.h"
#include "guidelines.h"
#include "track.h"
#include "vtrackrender.h"
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
	is_opaque = 0;
	vframe = 0;
	renderer = 0;
}

GuideFrame::~GuideFrame()
{
	delete [] data;
	delete vframe;
}

void GuideFrame::check_alloc(int num)
{
	int16_t *new_alloc;

	if(num > 0 && num + (dataend - data) > allocated)
	{
		canvas->lock_canvas("GuideFrame::check_alloc");
		int len = dataend - data + num;

		if(allocated + GUIDELINE_UNIT > len)
			len = allocated + GUIDELINE_UNIT;
		new_alloc = new int16_t[len];
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

void GuideFrame::set_opaque(int opaque)
{
	is_opaque = opaque;
}

VFrame *GuideFrame::get_vframe(int w, int h)
{
	canvas->lock_canvas("GuideFrame::get_vframe");
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
	int16_t *dp;
	int x1, x2, y1, y2;
	int in_x1, in_y1, in_x2, in_y2;
	int out_x1, out_y1, out_x2, out_y2;
	double xscale, yscale;
	double dx1, dx2, dy1, dy2;
	int pluginframe;
	BC_WindowBase *canvasbase = canvas->get_canvas();

	if(renderer && !renderer->media_track->play)
		return 0;

	if(is_enabled && data && start <= pts && pts < end)
	{
		if(canvasbase->opengl_active())
		{
			for(dp = data; dp < dataend;)
			{
				switch(*dp++)
				{
				case GUIDELINE_LINE:
					canvasbase->opengl_guideline(dp[0], dp[1],
						dp[2], dp[3],
						edlsession->output_w,
						edlsession->output_h,
						color, is_opaque);
					dp += 4;
					break;
				case GUIDELINE_RECTANGLE:
					canvasbase->opengl_guiderectangle(
						dp[0], dp[1], dp[2], dp[3],
						edlsession->output_w,
						edlsession->output_h,
						color, is_opaque);
					dp += 4;
					break;
				case GUIDELINE_BOX:
					canvasbase->opengl_guidebox(
						dp[0], dp[1], dp[2], dp[3],
						edlsession->output_w,
						edlsession->output_h,
						color, is_opaque);
					dp += 4;
					break;
				case GUIDELINE_DISC:
					canvasbase->opengl_guidedisc(
						dp[0], dp[1], dp[2], dp[3], color, is_opaque);
					dp += 4;
					break;
				case GUIDELINE_CIRCLE:
					canvasbase->opengl_guidecircle(
						dp[0], dp[1], dp[2], dp[3], color, is_opaque);
					dp += 4;
					break;
				case GUIDELINE_PIXEL:
					canvasbase->opengl_guidepixel(
						dp[0], dp[1], color, is_opaque);
					dp += 2;
					break;
				case GUIDELINE_VFRAME:
					canvasbase->opengl_guideframe(vframe,
						color, is_opaque);
					break;
				}
			}
			canvasbase->opengl_swapbuffers();
			canvasbase->opengl_release();
			return 1;
		}
		if(renderer && canvas->refresh_frame)
		{
			renderer->calculate_output_transfer(canvas->refresh_frame,
				&in_x1, &in_y1, &in_x2, &in_y2,
				&out_x1, &out_y1, &out_x2, &out_y2);
			xscale = (double)(out_x2 - out_x1) / (in_x2 - in_x1);
			yscale = (double)(out_y2 - out_y1) / (in_y2 - in_y1);
			pluginframe = 1;
		}
		else
		{
			xscale = yscale = 1;
			in_x1 = out_x1 = 0;
			in_y1 = out_y1 = 0;
			pluginframe = 0;
		}

		canvasbase->set_color(color);

		if(is_opaque)
			canvasbase->set_opaque();

		for(dp = data; dp < dataend;)
		{
			if(pluginframe)
			{
				dx1 = (dp[1] - in_x1) * xscale + out_x1;
				dy1 = (dp[2] - in_y1) * yscale + out_y1;
				canvas->output_to_canvas(dx1, dy1);
				x1 = round(dx1);
				y1 = round(dy1);

				if(*dp != GUIDELINE_PIXEL)
				{
					if(*dp == GUIDELINE_LINE)
					{
						dx2 = (dp[3] - in_x1) * xscale + out_x1;
						dy2 = (dp[4] - in_y1) * yscale + out_y1;
					}
					else
					{
						dx2 = (dp[3] + dp[1] - in_x1) *
							xscale + out_x1;
						dy2 = (dp[4] + dp[2] - in_y1) *
							yscale + out_y1;
					}
					canvas->output_to_canvas(dx2, dy2);
					x2 = round(dx2);
					y2 = round(dy2);
				}
			}
			else
			{
				dx1 = dp[1];
				dy1 = dp[2];
				canvas->output_to_canvas(dx1, dy1);
				x1 = round(dx1);
				y1 = round(dy1);
				if(*dp != GUIDELINE_PIXEL)
				{
					if(*dp == GUIDELINE_LINE)
					{
						dx2 = dp[3];
						dy2 = dp[4];
					}
					else
					{
						dx2 = dp[3] + dp[1];
						dy2 = dp[4] + dp[2];
					}
					canvas->output_to_canvas(dx2, dy2);
					x2 = round(dx2);
					y2 = round(dy2);
				}
			}

			switch(*dp++)
			{
			case GUIDELINE_LINE:
				canvasbase->draw_line(x1, y1, x2, y2);
				dp += 4;
				break;
			case GUIDELINE_RECTANGLE:
				canvasbase->draw_rectangle(x1, y1,
					x2 - x1, y2 - y1);
				dp += 4;
				break;
			case GUIDELINE_BOX:
				canvasbase->draw_box(x1, y1,
					x2 - x1, y2 - y1);
				dp += 4;
				break;
			case GUIDELINE_DISC:
				canvasbase->draw_disc(x1, y1,
					x2 - x1, y2 - y1);
				dp += 4;
				break;
			case GUIDELINE_CIRCLE:
				canvasbase->draw_circle(x1, y1,
					x2 - x1, y2 - y1);
				dp += 4;
				break;
			case GUIDELINE_PIXEL:
				canvasbase->draw_pixel(x1, y1);
				dp += 2;
				break;
			case GUIDELINE_VFRAME:
				if(vframe)
				{
					dx1 = in_x1;
					dy1 = in_y1;
					canvas->output_to_canvas(dx1, dy1);
					dx2 = in_x2;
					dy2 = in_y2;
					canvas->output_to_canvas(dx2, dy2);
					x1 = round(dx1);
					y1 = round(dy1);
					x2 = round(dx2);
					y2 = round(dy2);
					vframe->set_pixel_aspect(canvas->sample_aspect_ratio());
					canvasbase->draw_vframe(vframe,
						x1, y1,
						x2 - x1, y2 - y1,
						in_x1, in_y1,
						in_x2 - in_x1, in_y2 - in_y1,
						0);
				}
				break;
			default:
				break;
			}
		}

		if(is_opaque)
			canvasbase->set_inverse();

		if(period)
		{
			period_count = period;
			this->edl = edl;
			this->pts = pts;
		}
		return 1;
	}
	if(canvasbase->opengl_active())
	{
		canvasbase->opengl_swapbuffers();
		canvasbase->opengl_release();
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
	int16_t *p;
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

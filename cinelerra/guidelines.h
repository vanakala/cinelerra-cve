// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2018 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef GUIDELINES_H
#define GUIDELINES_H

#include "bcwindowbase.inc"
#include "canvas.inc"
#include "datatype.h"
#include "edl.inc"
#include "guidelines.inc"
#include "linklist.h"
#include "vtrackrender.inc"
#include "vframe.inc"

class GuideFrame : public ListItem<GuideFrame>
{
public:
	GuideFrame(ptstime start, ptstime end, Canvas *canvas);
	~GuideFrame();

	void check_alloc(int bytes);
	void add_line(int x1, int y1, int x2, int y2);
	void add_rectangle(int x1, int y1, int w, int h);
	void add_box(int x1, int y1, int w, int h);
	void add_disc(int x, int y, int w, int h);
	void add_circle(int x, int y, int w, int h);
	void add_pixel(int x, int y);
	void add_vframe();
	void clear();
	int set_enabled(int value);
	void set_repeater_period(int period);
	void shift(ptstime difference);
	void set_position(ptstime new_start, ptstime new_end);
	void set_color(int color);
	void set_opaque(int opaque);
	VFrame *get_vframe(int w, int h);
	void repeat_event(Canvas *canvas);
	int draw(Canvas *canvas, EDL *edl, ptstime pts);
	int has_repeater_period();
	size_t get_size();
	void dump(int indent = 0);

	VTrackRender *renderer;
private:
	ptstime start;
	ptstime end;
	int period;
	int period_count;
	int is_enabled;
	int is_opaque;
	EDL *edl;
	ptstime pts;
	int color;
	int allocated;
	int16_t *dataend;
	int16_t *data;
	VFrame *vframe;
	Canvas *canvas;
};

class GuideLines : public List<GuideFrame>
{
public:
	GuideLines();
	~GuideLines();

	void set_canvas(Canvas *canvas);
	void delete_guideframes();
	GuideFrame *append_frame(ptstime start, ptstime end);
	void draw(EDL *edl, ptstime pts);
	void activate_repeater();
	void stop_repeater();
	void repeat_event(int period);
	size_t get_size();
	void dump(int indent = 0);
private:
	BC_WindowBase *repeater_window;
	Canvas *canvas;
};

#endif

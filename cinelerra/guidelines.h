
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

#ifndef GUIDELINES_H
#define GUIDELINES_H

#include "canvas.inc"
#include "datatype.h"
#include "edl.inc"
#include "guidelines.inc"
#include "linklist.h"

class GuideFrame : public ListItem<GuideFrame>
{
public:
	GuideFrame(ptstime start, ptstime end);
	~GuideFrame();

	void check_alloc(int bytes);
	void add_line(int x1, int y1, int x2, int y2);
	void add_rectangle(int x1, int y1, int w, int h);
	void add_box(int x1, int y1, int w, int h);
	void add_disc(int x, int y, int w, int h);
	void add_circle(int x, int y, int w, int h);
	void add_pixel(int x, int y);
	void clear();
	int set_enabled(int value);
	void shift(ptstime difference);
	void set_position(ptstime new_start, ptstime new_end);
	void draw(Canvas *canvas, EDL *edl, ptstime pts);
	void dump(int indent = 0);

private:
	ptstime start;
	ptstime end;
	int period;
	int is_enabled;
	int allocated;
	uint16_t *dataend;
	uint16_t *data;
};

class GuideLines : public List<GuideFrame>
{
public:
	GuideLines();
	~GuideLines();

	void delete_guideframes();
	GuideFrame *append_frame(ptstime start, ptstime end);
	void draw(Canvas *canvas, EDL *edl, ptstime pts);
	void dump(int indent = 0);
};

#endif

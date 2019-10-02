
/*
 * CINELERRA
 * Copyright (C) 2013 Einar Rünkaru <einarrunkaru@gmail dot com>
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

#ifndef RULER_H
#define RULER_H

#include "bcwindow.h"
#include "mwindow.inc"
#include "ruler.inc"
#include "thread.h"

class RulerGUI;

class Ruler : public Thread
{
public:
	Ruler(MWindow *mwindow);
	~Ruler();

	void run();

	RulerGUI *gui;
};

class RulerGUI : public BC_Window
{
public:
	RulerGUI(MWindow *mwindow, Ruler *thread);

	void draw_ruler();
	void resize_event(int w, int h);
	void translation_event();
	void close_event();
	int keypress_event();
	int button_release_event();
	int drag_start_event();
	int cursor_motion_event();
	void drag_motion_event();
	void drag_stop_event();

private:
	int ticklength(int tick);

	MWindow *mwindow;
	int resize;
	int width;
	int height;
	// center of closing button
	int x_cl;
	int y_cl;

	int max_length;
	int min_length;
	int root_w;
	int root_h;
	int separate_cursor;
};

#endif

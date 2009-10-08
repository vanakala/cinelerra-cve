
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

#ifndef VIDEOWINDOWGUI_H
#define VIDEOWINDOWGUI_H

#include "guicast.h"
#include "videowindow.inc"

class VideoWindowGUI : public BC_Window
{
public:
	VideoWindowGUI(VideoWindow *thread, int w, int h);
	~VideoWindowGUI();

	int create_objects();
	int resize_event(int w, int h);
	int close_event();
	int keypress_event();
	int update_title();
	int start_cropping();
	int stop_cropping();

	int x1, y1, x2, y2, center_x, center_y;
	int x_offset, y_offset;
	VideoWindow *thread;
	VideoWindowCanvas *canvas;
};

class VideoWindowCanvas : public BC_SubWindow
{
public:
	VideoWindowCanvas(VideoWindowGUI *gui, int w, int h);
	~VideoWindowCanvas();

	int button_press();
	int button_release();
	int cursor_motion();
	int draw_crop_box();

	int button_down;
	VideoWindowGUI *gui;
	int corner_selected;
};


#endif

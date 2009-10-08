
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

#ifndef VIDEOWINDOW_H
#define VIDEOWINDOW_H


#include "bchash.inc"
#include "mwindow.inc"
#include "thread.h"
#include "vframe.inc"
#include "videowindowgui.inc"


class VideoWindow : public Thread
{
public:
	VideoWindow(MWindow *mwindow);
	~VideoWindow();
	
	int create_objects();
	int init_window();
	int load_defaults(BC_Hash *defaults);
	int update_defaults(BC_Hash *defaults);
	int get_aspect_ratio(float &aspect_w, float &aspect_h);
	int fix_size(int &w, int &h, int width_given, float aspect_ratio);
	int get_full_sizes(int &w, int &h);
	void run();

	int show_window();
	int hide_window();
	int resize_window();
	int original_size(); // Put the window at its original size
	int reset();
	int init_video();
	int stop_video();
	int update(BC_Bitmap *frame);
	int get_w();
	int get_h();
	int start_cropping();
	int stop_cropping();
	BC_Bitmap* get_bitmap();  // get a bitmap for playback

// allocated according to playback buffers
	float **peak_history;

	int video_visible;
	int video_cropping;    // Currently performing a cropping operation
//	float zoom_factor;
	int video_window_w;    // Horizontal size of the window independant of frame size
	VFrame **vbuffer;      // output frame buffer
	VideoWindowGUI *gui;
	MWindow *mwindow;
};





#endif

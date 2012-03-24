
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

#ifndef RESOURCEPIXMAP_H
#define RESOURCEPIXMAP_H

#include "bctimer.inc"
#include "edit.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "trackcanvas.inc"


// Can't use garbage collection for GUI elements because they need to
// lock the window for deletion.
class ResourcePixmap : public BC_Pixmap
{
public:
	ResourcePixmap(MWindow *mwindow, 
		TrackCanvas *canvas, 
		Edit *edit, 
		int w, 
		int h);
	~ResourcePixmap();

	void resize(int w, int h);
	void draw_data(Edit *edit,
		int edit_x,
		int edit_w,
		int pixmap_x,
		int pixmap_w,
		int pixmap_h,
		int mode,
		int indexes_only);
	void draw_audio_resource(Edit *edit, 
		int x, 
		int w);
	void draw_video_resource(Edit *edit, 
		int edit_x,
		int edit_w,
		int pixmap_x,
		int pixmap_w,
		int refresh_x, 
		int refresh_w,
		int mode);
	void draw_audio_source(Edit *edit, int x, int w);
// Called by ResourceThread to update pixmap
	void draw_wave(int x, double high, double low);
	void draw_title(Edit *edit, int edit_x, int edit_w, int pixmap_x, int pixmap_w);
// Change to hourglass if timer expired
	void test_timer();

	void dump();

	MWindow *mwindow;
	TrackCanvas *canvas;
// Visible in entire track canvas
	int visible;
// Section drawn
	int edit_id;
	int edit_x, pixmap_x, pixmap_w, pixmap_h;
	int64_t zoom_track, zoom_y;
	ptstime zoom_time;
	ptstime source_pts;
	int data_type;
// Timer to cause an hourglass to appear
	Timer *timer;
};

#endif

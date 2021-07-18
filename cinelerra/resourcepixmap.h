// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef RESOURCEPIXMAP_H
#define RESOURCEPIXMAP_H

#include "aframe.inc"
#include "bcpixmap.h"
#include "edit.inc"
#include "resourcethread.inc"
#include "trackcanvas.inc"


class ResourcePixmap : public BC_Pixmap
{
public:
	ResourcePixmap(ResourceThread *resource_thread,
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
		int mode);
	void draw_audio_resource(Edit *edit, 
		int x, 
		int w);
	void draw_video_resource(Edit *edit, 
		int edit_x,
		int edit_w,
		int pixmap_x,
		int pixmap_w,
		int refresh_x,
		int refresh_w);
	void draw_audio_source(Edit *edit, int x, int w);
// Called by ResourceThread to update pixmap
	void draw_wave(int x, double high, double low);
	void draw_title(Edit *edit, int edit_x, int edit_w, int pixmap_x, int pixmap_w);
// Change to hourglass if timer expired
	void test_timer();

	void dump(int indent);

	TrackCanvas *canvas;
	int edit_x, pixmap_x, pixmap_w, pixmap_h;
// Visible in entire track canvas
	int visible;
	int edit_id;

private:
	ResourceThread *resource_thread;
	int zoom_track, zoom_y;
	ptstime zoom_time;
	ptstime source_pts;
	int data_type;
	AFrame *aframe;
	ptstime last_picon_size;
	int num_picons;
};

#endif

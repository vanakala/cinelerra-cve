
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

#ifndef RESIZETRACKTHREAD_H
#define RESIZETRACKTHREAD_h





#include "guicast.h"
#include "mutex.inc"
#include "mwindow.inc"
#include "thread.h"
#include "track.inc"


class ResizeTrackWindow;

class ResizeTrackThread : public Thread
{
public:
	ResizeTrackThread(MWindow *mwindow, int track_number);
	~ResizeTrackThread();

	void start_window(Track *track, int track_number);
	void run();

	ResizeTrackWindow *window;
	MWindow *mwindow;
	int track_number;
	int w, h;
	int w1, h1;
	double w_scale, h_scale;
};



class ResizeTrackWindow;


class ResizeTrackWidth : public BC_TextBox
{
public:
	ResizeTrackWidth(ResizeTrackWindow *gui, 
		ResizeTrackThread *thread,
		int x,
		int y);
	int handle_event();
	ResizeTrackWindow *gui;
	ResizeTrackThread *thread;
};

class ResizeTrackSwap : public BC_Button
{
public:
	ResizeTrackSwap(ResizeTrackWindow *gui, 
		ResizeTrackThread *thread, 
		int x, 
		int y);
	int handle_event();
	ResizeTrackWindow *gui;
	ResizeTrackThread *thread;
};

class ResizeTrackHeight : public BC_TextBox
{
public:
	ResizeTrackHeight(ResizeTrackWindow *gui, 
		ResizeTrackThread *thread,
		int x,
		int y);
	int handle_event();
	ResizeTrackWindow *gui;
	ResizeTrackThread *thread;
};


class ResizeTrackScaleW : public BC_TextBox
{
public:
	ResizeTrackScaleW(ResizeTrackWindow *gui, 
		ResizeTrackThread *thread,
		int x,
		int y);
	int handle_event();
	ResizeTrackWindow *gui;
	ResizeTrackThread *thread;
};

class ResizeTrackScaleH : public BC_TextBox
{
public:
	ResizeTrackScaleH(ResizeTrackWindow *gui, 
		ResizeTrackThread *thread,
		int x,
		int y);
	int handle_event();
	ResizeTrackWindow *gui;
	ResizeTrackThread *thread;
};


class ResizeTrackWindow : public BC_Window
{
public:
	ResizeTrackWindow(MWindow *mwindow, 
		ResizeTrackThread *thread,
		int x,
		int y);
	~ResizeTrackWindow();
	
	void create_objects();
	void update(int changed_scale, 
		int changed_size, 
		int changed_all);
	
	MWindow *mwindow;
	ResizeTrackThread *thread;
	ResizeTrackWidth *w;
	ResizeTrackHeight *h;
	ResizeTrackScaleW *w_scale;
	ResizeTrackScaleH *h_scale;
};



#endif

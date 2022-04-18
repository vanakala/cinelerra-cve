// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef RESIZETRACKTHREAD_H
#define RESIZETRACKTHREAD_h

#include "mutex.inc"
#include "mwindow.inc"
#include "resizetrackthread.inc"
#include "selection.h"
#include "track.inc"


class ResizeTrackWindow;

class ResizeTrackThread : public Thread
{
public:
	ResizeTrackThread();
	~ResizeTrackThread();

	void start_window(Track *track);
private:
	void run();

public:
	ResizeTrackWindow *window;
	Track *track;
	int w, h;
	int w1, h1;
	double w_scale, h_scale;
};

class ResizeTrackScaleW : public BC_TextBox
{
public:
	ResizeTrackScaleW(ResizeTrackWindow *gui, 
		ResizeTrackThread *thread,
		int x,
		int y);

	int handle_event();
private:
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
private:
	ResizeTrackWindow *gui;
	ResizeTrackThread *thread;
};


class ResizeTrackWindow : public BC_Window
{
public:
	ResizeTrackWindow(ResizeTrackThread *thread,
		int x,
		int y);

	void update(int changed_scale,
		int changed_size);
private:
	ResizeTrackThread *thread;
	FrameSizeSelection *framesize_selection;
	ResizeTrackScaleW *w_scale;
	ResizeTrackScaleH *h_scale;
};


class SetTrackFrameSize : public FrameSizeSelection
{
public:
	SetTrackFrameSize(int x1, int y1, int x2, int y2,
		ResizeTrackWindow *base, int *value1, int *value2);

	int handle_event();
private:
	ResizeTrackWindow *gui;
};

#endif

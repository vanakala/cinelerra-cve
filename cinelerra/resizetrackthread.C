// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcbutton.h"
#include "bctitle.h"
#include "edlsession.h"
#include "language.h"
#include "mainerror.h"
#include "mwindow.h"
#include "resizetrackthread.h"
#include "selection.h"
#include "track.h"

ResizeTrackThread::ResizeTrackThread()
 : Thread()
{
	window = 0;
}

ResizeTrackThread::~ResizeTrackThread()
{
	if(window)
		window->set_done(1);

	Thread::join();
}

void ResizeTrackThread::start_window(Track *track)
{
	this->track = track;
	w1 = w = track->track_w;
	h1 = h = track->track_h;
	w_scale = h_scale = 1;
	start();
}

void ResizeTrackThread::run()
{
	int cx, cy;

	mwindow_global->get_abs_cursor_pos(&cx, &cy);
	ResizeTrackWindow *window = this->window =
		new ResizeTrackWindow(this, cx, cy);
	int result = window->run_window();
	this->window = 0;
	delete window;

	if(!result && track)
	{
		FrameSizeSelection::limits(&w, &h);
		mwindow_global->resize_track(track, w, h);
	}
}

#define TRWIN_INTERVAL 25
#define TRWIN_BOXLEFT  75

ResizeTrackWindow::ResizeTrackWindow(ResizeTrackThread *thread,
	int x,
	int y)
 : BC_Window(MWindow::create_title(N_("Resize Track")),
		x - 350 / 2,
		y - BC_OKButton::calculate_h() + 100 / 2,
		350,
		BC_OKButton::calculate_h() + 100,
		350,
		BC_OKButton::calculate_h() + 100,
		0,
		0,
		1)
{
	const char *mul = "x";

	this->thread = thread;

	x = 10, y = 10;

	set_icon(mwindow_global->get_window_icon());
	add_subwindow(new BC_Title(x, y, _("Size:")));
	x += TRWIN_BOXLEFT;

	add_subwindow(framesize_selection = new SetTrackFrameSize(x, y,
		x + SELECTION_TB_WIDTH + TRWIN_INTERVAL, y,
		this, &thread->w, &thread->h));
	framesize_selection->update(thread->w, thread->h);

	y += 30;
	x = 10;
	add_subwindow(new BC_Title(x, y, _("Scale:")));
	x += TRWIN_BOXLEFT;
	add_subwindow(w_scale = new ResizeTrackScaleW(this, 
		thread,
		x,
		y));
	int v = (TRWIN_INTERVAL - get_text_width(MEDIUMFONT, mul, 1)) / 2;
	x += SELECTION_TB_WIDTH + v;
	add_subwindow(new BC_Title(x, y, mul));
	x += TRWIN_INTERVAL - v;
	add_subwindow(h_scale = new ResizeTrackScaleH(this, 
		thread,
		x,
		y));

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));

	show_window();
}

void ResizeTrackWindow::update(int changed_scale, 
	int changed_size)
{
	if(changed_scale)
	{
		thread->w = round(thread->w1 * thread->w_scale);
		thread->h = round(thread->h1 * thread->h_scale);
		framesize_selection->update(thread->w, thread->h);
	}
	else
	if(changed_size)
	{
		thread->w_scale = (double)thread->w / thread->w1;
		w_scale->update(thread->w_scale);
		thread->h_scale = (double)thread->h / thread->h1;
		h_scale->update(thread->h_scale);
	}
}


ResizeTrackScaleW::ResizeTrackScaleW(ResizeTrackWindow *gui, 
	ResizeTrackThread *thread,
	int x,
	int y)
 : BC_TextBox(x, y, SELECTION_TB_WIDTH, 1, thread->w_scale)
{
	this->gui = gui;
	this->thread = thread;
}

int ResizeTrackScaleW::handle_event()
{
	thread->w_scale = atof(get_text());
	gui->update(1, 0);
	return 1;
}

ResizeTrackScaleH::ResizeTrackScaleH(ResizeTrackWindow *gui, 
	ResizeTrackThread *thread,
	int x,
	int y)
 : BC_TextBox(x, y, SELECTION_TB_WIDTH, 1, thread->h_scale)
{
	this->gui = gui;
	this->thread = thread;
}

int ResizeTrackScaleH::handle_event()
{
	thread->h_scale = atof(get_text());
	gui->update(1, 0);
	return 1;
}

SetTrackFrameSize::SetTrackFrameSize(int x1, int y1, int x2, int y2,
	ResizeTrackWindow *base, int *value1, int *value2)
 : FrameSizeSelection(x1, y1, x2, y2, base, value1, value2)
{
	this->gui = base;
}

int SetTrackFrameSize::handle_event()
{
	Selection::handle_event();
	gui->update(0, 1);
	return 1;
}

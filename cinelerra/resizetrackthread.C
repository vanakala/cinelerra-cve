#include "edl.h"
#include "language.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "new.h"
#include "resizetrackthread.h"
#include "track.h"
#include "tracks.h"






ResizeTrackThread::ResizeTrackThread(MWindow *mwindow, int track_number)
 : Thread()
{
	this->mwindow = mwindow;
	this->track_number = track_number;
	window = 0;
}

ResizeTrackThread::~ResizeTrackThread()
{
	if(window)
	{
		window->lock_window();
		window->set_done(1);
		window->unlock_window();
	}

	Thread::join();
}

void ResizeTrackThread::start_window(Track *track, int track_number)
{
	this->track_number = track_number;
	w1 = w = track->track_w;
	h1 = h = track->track_h;
	w_scale = h_scale = 1;
	start();
}


void ResizeTrackThread::run()
{
	ResizeTrackWindow *window = this->window = 
		new ResizeTrackWindow(mwindow, 
			this,
			mwindow->gui->get_abs_cursor_x(1),
			mwindow->gui->get_abs_cursor_y(1));
	window->create_objects();
	int result = window->run_window();
	this->window = 0;
	delete window;


	if(!result)
	{
		Track *track = mwindow->edl->tracks->get_item_number(track_number);

		if(track)
		{
			mwindow->resize_track(track, w, h);
		}
	}
}




ResizeTrackWindow::ResizeTrackWindow(MWindow *mwindow, 
	ResizeTrackThread *thread,
	int x,
	int y)
 : BC_Window(PROGRAM_NAME ": Resize Track", 
				x - 320 / 2,
				y - 120 / 2,
				320, 
				120, 
				-1, 
				-1, 
				0,
				0, 
				1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

ResizeTrackWindow::~ResizeTrackWindow()
{
}

void ResizeTrackWindow::create_objects()
{
	int x = 10, y = 10;

	add_subwindow(new BC_Title(x, y, _("Size:")));
	x += 50;
	add_subwindow(w = new ResizeTrackWidth(this, 
		thread,
		x,
		y));
	x += 100;
	add_subwindow(new BC_Title(x, y, _("x")));
	x += 15;
	add_subwindow(h = new ResizeTrackHeight(this, 
		thread,
		x,
		y));
	x += 100;
	add_subwindow(new FrameSizePulldown(mwindow, 
		w, 
		h, 
		x, 
		y));

	y += 30;
	x = 10;
	add_subwindow(new BC_Title(x, y, _("Scale:")));
	x += 50;
	add_subwindow(w_scale = new ResizeTrackScaleW(this, 
		thread,
		x,
		y));
	x += 100;
	add_subwindow(new BC_Title(x, y, _("x")));
	x += 15;
	add_subwindow(h_scale = new ResizeTrackScaleH(this, 
		thread,
		x,
		y));

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));

	show_window();
	flush();
}

void ResizeTrackWindow::update(int changed_scale, 
	int changed_size, 
	int changed_all)
{
//printf("ResizeTrackWindow::update %d %d %d\n", changed_scale, changed_size, changed_all);
	if(changed_scale || changed_all)
	{
		thread->w = (int)(thread->w1 * thread->w_scale);
		w->update((int64_t)thread->w);
		thread->h = (int)(thread->h1 * thread->h_scale);
		h->update((int64_t)thread->h);
	}
	else
	if(changed_size || changed_all)
	{
		thread->w_scale = (double)thread->w / thread->w1;
		w_scale->update((float)thread->w_scale);
		thread->h_scale = (double)thread->h / thread->h1;
		h_scale->update((float)thread->h_scale);
	}
}





ResizeTrackWidth::ResizeTrackWidth(ResizeTrackWindow *gui, 
	ResizeTrackThread *thread,
	int x,
	int y)
 : BC_TextBox(x, y, 90, 1, thread->w)
{
	this->gui = gui;
	this->thread = thread;
}
int ResizeTrackWidth::handle_event()
{
	thread->w = atol(get_text());
	gui->update(0, 1, 0);
	return 1;
}

ResizeTrackHeight::ResizeTrackHeight(ResizeTrackWindow *gui, 
	ResizeTrackThread *thread,
	int x,
	int y)
 : BC_TextBox(x, y, 90, 1, thread->h)
{
	this->gui = gui;
	this->thread = thread;
}
int ResizeTrackHeight::handle_event()
{
	thread->h = atol(get_text());
	gui->update(0, 1, 0);
	return 1;
}


ResizeTrackScaleW::ResizeTrackScaleW(ResizeTrackWindow *gui, 
	ResizeTrackThread *thread,
	int x,
	int y)
 : BC_TextBox(x, y, 90, 1, (float)thread->w_scale)
{
	this->gui = gui;
	this->thread = thread;
}
int ResizeTrackScaleW::handle_event()
{
	thread->w_scale = atof(get_text());
	gui->update(1, 0, 0);
	return 1;
}

ResizeTrackScaleH::ResizeTrackScaleH(ResizeTrackWindow *gui, 
	ResizeTrackThread *thread,
	int x,
	int y)
 : BC_TextBox(x, y, 90, 1, (float)thread->h_scale)
{
	this->gui = gui;
	this->thread = thread;
}
int ResizeTrackScaleH::handle_event()
{
	thread->h_scale = atof(get_text());
	gui->update(1, 0, 0);
	return 1;
}








#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "preferences.h"
#include "mainsession.h"
#include "setframerate.h"
#include "tracks.h"

SetFrameRate::SetFrameRate(MWindow *mwindow)
 : BC_MenuItem("Frame rate...")
{ 
	this->mwindow = mwindow;
	thread = new SetFrameRateThread(mwindow);
}

SetFrameRate::~SetFrameRate()
{
	delete thread;
}
 
int SetFrameRate::handle_event() { thread->start(); }

SetFrameRateThread::SetFrameRateThread(MWindow *mwindow) : Thread()
{
	this->mwindow = mwindow;
}

SetFrameRateThread::~SetFrameRateThread() {}

void SetFrameRateThread::run()
{
	frame_rate = mwindow->session->frame_rate;
//	scale_data = mwindow->defaults->get("FRAMERATESCALEDATA", 0);
	scale_data = 1;

	SetFrameRateWindow window(mwindow, this);
	window.create_objects();
	int result = window.run_window();
	if(!result)
	{
		mwindow->undo->update_undo_edits("Frame rate", 0);
		if(scale_data) mwindow->tracks->scale_time(frame_rate / mwindow->session->frame_rate, 1, 1, 1, 0, mwindow->tracks->total_samples());
		mwindow->session->frame_rate = frame_rate;
		mwindow->preferences->actual_frame_rate = frame_rate;
		mwindow->undo->update_undo_edits();
		mwindow->redraw_time_dependancies();
	}

//	mwindow->defaults->update("FRAMERATESCALEDATA", scale_data);
}


SetFrameRateWindow::SetFrameRateWindow(MWindow *mwindow, SetFrameRateThread *thread)
 : BC_Window(PROGRAM_NAME ": Frame Rate", 
 	mwindow->gui->get_abs_cursor_x(), 
	mwindow->gui->get_abs_cursor_y(), 
	340, 
	140)
{
	this->thread = thread;
}

SetFrameRateWindow::~SetFrameRateWindow()
{
}

int SetFrameRateWindow::create_objects()
{
	add_subwindow(new BC_Title(5, 5, "Enter the frame rate to use:"));
	add_subwindow(new BC_OKButton(10, get_h() - 40));
	add_subwindow(new BC_CancelButton(get_w() - 100, get_h() - 40));

	add_subwindow(new SetFrameRateTextBox(thread));
}

SetFrameRateTextBox::SetFrameRateTextBox(SetFrameRateThread *thread)
 : BC_TextBox(10, 40, 100, 1, thread->frame_rate)
{
	this->thread = thread;
}

int SetFrameRateTextBox::handle_event()
{
	thread->frame_rate = atof(get_text());
}

SetFrameRateMoveData::SetFrameRateMoveData(SetFrameRateThread *thread)
 : BC_CheckBox(120, 40, thread->scale_data, "Scale data")
{
	this->thread = thread;
}

int SetFrameRateMoveData::handle_event()
{
	thread->scale_data = get_value();
}

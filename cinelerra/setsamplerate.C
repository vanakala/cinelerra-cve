#include "mwindow.h"
#include "mwindowgui.h"
#include "mainsession.h"
#include "setsamplerate.h"

SetSampleRate::SetSampleRate(MWindow *mwindow)
 : BC_MenuItem("Sample rate...", ""), Thread() 
{ this->mwindow = mwindow; }
 
int SetSampleRate::handle_event() { start(); }

void SetSampleRate::run()
{
	SetSampleRateWindow window(mwindow, this);
	sample_rate = mwindow->session->sample_rate;
	window.create_objects();
	int result = window.run_window();
	if(!result)
	{
		if(sample_rate >= 4000 && sample_rate <= 200000)
		{
			mwindow->session->sample_rate = sample_rate;
			mwindow->session->changes_made = 1;
			mwindow->redraw_time_dependancies();
		}
	}
}


SetSampleRateWindow::SetSampleRateWindow(MWindow *mwindow, SetSampleRate *thread)
 : BC_Window(PROGRAM_NAME ": Sample Rate", 
 	mwindow->gui->get_abs_cursor_x(), 
	mwindow->gui->get_abs_cursor_y(), 
	340, 
	140)
{
	this->thread = thread;
	this->mwindow = mwindow;
}

SetSampleRateWindow::~SetSampleRateWindow()
{
}

int SetSampleRateWindow::create_objects()
{
	add_subwindow(new BC_Title(5, 5, "Enter the sample rate to use:"));
	add_subwindow(new BC_OKButton(10, get_h() - 40));
	add_subwindow(new BC_CancelButton(get_w() - 100, get_h() - 40));
	
	char string[1024];
	sprintf(string, "%d", thread->sample_rate);
	add_subwindow(new SetSampleRateTextBox(mwindow, this, string));
}

SetSampleRateTextBox::SetSampleRateTextBox(MWindow *mwindow, SetSampleRateWindow *srwindow, char *text)
 : BC_TextBox(10, 40, 100, 1, text)
{
	this->mwindow = mwindow;
	this->srwindow = srwindow;
}

int SetSampleRateTextBox::handle_event()
{
	srwindow->thread->sample_rate = atol(get_text());
}

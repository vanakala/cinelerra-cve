#ifndef SETSAMPLERATE_H
#define SETSAMPLERATE_H

#include "guicast.h"
#include "mwindow.inc"
#include "thread.h"

class SetSampleRate : public BC_MenuItem, public Thread
{
public:
	SetSampleRate(MWindow *mwindow);
	int handle_event();
	void run();
	
	MWindow *mwindow;
	long sample_rate;
};

class SetSampleRateWindow : public BC_Window
{
public:
	SetSampleRateWindow(MWindow *mwindow, SetSampleRate *thread);
	~SetSampleRateWindow();
	int create_objects();
	
	MWindow *mwindow;
	SetSampleRate *thread;
};
/*
 * 
 * class SetSampleRateTextBox : public BC_TextBox
 * {
 * public:
 * 	SetSampleRateTextBox(MWindow *mwindow, SetSampleRateWindow *srwindow, char *text);
 * 	
 * 	int handle_event();
 * 	
 * 	SetSampleRateWindow *srwindow;
 * 	MWindow *mwindow;
 * };
 */

#endif


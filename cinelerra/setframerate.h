#ifndef SETFRAMERATE_H
#define SETFRAMERATE_H

#include "guicast.h"
#include "setframerate.inc"
#include "thread.h"

class SetFrameRate : public BC_MenuItem
{
public:
	SetFrameRate(MWindow *mwindow);
	~SetFrameRate();

	int handle_event();
	
	MWindow *mwindow;
	SetFrameRateThread *thread;
};

class SetFrameRateThread : public Thread
{
public:
	SetFrameRateThread(MWindow *mwindow);
	~SetFrameRateThread();

	void run();

	MWindow *mwindow;
	float frame_rate;
	int scale_data;
};

class SetFrameRateWindow : public BC_Window
{
public:
	SetFrameRateWindow(MWindow *mwindow, SetFrameRateThread *thread);
	~SetFrameRateWindow();
	int create_objects();

	SetFrameRateThread *thread;
};

class SetFrameRateMoveData : public BC_CheckBox
{
public:
	SetFrameRateMoveData(SetFrameRateThread *thread);
	int handle_event();
	SetFrameRateThread *thread;
};

#endif


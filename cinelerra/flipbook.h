#ifndef FLIPBOOK_H
#define FLIPBOOK_H

#include "guicast.h"
#include "thread.h"

class FlipBookItem
{
public:
	Asset asset;
	long duration;     // samples or frames depending on the asset
};


class FlipBook : public BC_MenuItem
{
public:
	FlipBook(MWindow *mwindow);
	~FlipBook();

	handle_event();

	MWindow *mwindow;
};

class FlipBookThread : public Thread
{
public:
	FlipBookThread(MWindow *mwindow);
	~FlipBookThread();

	void run();

	MWindow *mwindow;
};

class FlipBookGUI : public BC_Window
{
public:
	FlipBookGUI(FlipBookThread *thread, MWindow *mwindow);
	~FlipBookGUI();

	FlipBookThread *thread;
	MWindow *mwindow;
};




#endif

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include "guicast.h"
#include "mwindow.inc"
#include "mwindowgui.inc"

class StatusBarCancel;

class StatusBar : public BC_SubWindow
{
public:
	StatusBar(MWindow *mwindow, MWindowGUI *gui);
	~StatusBar();
	
	void set_message(char *text);
	void default_message();
	int create_objects();
	void resize_event();
	
	MWindow *mwindow;
	MWindowGUI *gui;
	BC_ProgressBar *main_progress;
	StatusBarCancel *main_progress_cancel;
	BC_Title *status_text;
};

class StatusBarCancel : public BC_Button
{
public:
	StatusBarCancel(MWindow *mwindow, int x, int y);

	int handle_event();

	MWindow *mwindow;
};

#endif

#ifndef TIPWINDOW_H
#define TIPWINDOW_H

#include "bcdialog.h"
#include "guicast.h"
#include "mwindow.inc"
#include "tipwindow.inc"



// Tip of the day to be run at initialization


class TipWindow : public BC_DialogThread
{
public:
	TipWindow(MWindow *mwindow);

	BC_Window* new_gui();
	char* get_current_tip();
	void next_tip();
	void prev_tip();

	MWindow *mwindow;
	TipWindowGUI *gui;
};


class TipWindowGUI : public BC_Window
{
public:
	TipWindowGUI(MWindow *mwindow, 
		TipWindow *thread,
		int x,
		int y);
	void create_objects();
	int keypress_event();
	MWindow *mwindow;
	TipWindow *thread;
	BC_Title *tip_text;
};

class TipDisable : public BC_CheckBox
{
public:
	TipDisable(MWindow *mwindow, TipWindowGUI *gui, int x, int y);
	int handle_event();
	TipWindowGUI *gui;
	MWindow *mwindow;
};

class TipNext : public BC_Button
{
public:
	TipNext(MWindow *mwindow, TipWindowGUI *gui, int x, int y);
	int handle_event();
	static int calculate_w(MWindow *mwindow);
	TipWindowGUI *gui;
	MWindow *mwindow;
};

class TipPrev : public BC_Button
{
public:
	TipPrev(MWindow *mwindow, TipWindowGUI *gui, int x, int y);
	int handle_event();
	static int calculate_w(MWindow *mwindow);
	TipWindowGUI *gui;
	MWindow *mwindow;
};

class TipClose : public BC_Button
{
public:
	TipClose(MWindow *mwindow, TipWindowGUI *gui, int x, int y);
	int handle_event();
	static int calculate_w(MWindow *mwindow);
	static int calculate_h(MWindow *mwindow);
	TipWindowGUI *gui;
	MWindow *mwindow;
};



#endif

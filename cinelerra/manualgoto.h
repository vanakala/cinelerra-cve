#ifndef MANUALGOTO_H
#define MANUALGOTO_H

#include "awindow.inc"
#include "edl.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "thread.h"
#include "vwindow.inc"
#include "editpanel.inc"

class ManualGotoWindow;
class ManualGotoNumber;

class ManualGoto : public Thread
{
public:
	ManualGoto(MWindow *mwindow, BC_WindowBase *masterwindow);
	~ManualGoto();

	void run();

// If it is being created or edited
	MWindow *mwindow;
	BC_WindowBase *masterwindow;
	void open_window();

	ManualGotoWindow *gotowindow;
	int done;

};




class ManualGotoWindow : public BC_Window
{
public:
	ManualGotoWindow(MWindow *mwindow, ManualGoto *thread);
	~ManualGotoWindow();

	void create_objects();
	int activate();
	void reset_data(double position);
	double get_entered_position_sec();
	void set_entered_position_sec(double position);



// Use this copy of the pointer in ManualGoto since multiple windows are possible	
	BC_Title *signtitle;
	ManualGotoNumber *boxhours;
	ManualGotoNumber *boxminutes;
	ManualGotoNumber *boxseconds;
	ManualGotoNumber *boxmsec;
	MWindow *mwindow;
	ManualGoto *thread;
};



class ManualGotoNumber : public BC_TextBox
{
public:
	ManualGotoNumber(ManualGotoWindow *window, int x, int y, int w, int min_num, int max_num, int chars);
	int handle_event();
	ManualGotoWindow *window;
	int keypress_event();
	int activate();
	int deactivate();
	void reshape_update(int64_t number);
	
	int min_num;
	int max_num;
	int chars;
};







#endif

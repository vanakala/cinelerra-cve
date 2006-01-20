#ifndef LABELEDIT_H
#define LABELEDIT_H

#include "awindow.inc"
#include "edl.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "thread.h"
#include "vwindow.inc"


class LabelEdit : public Thread
{
public:
	LabelEdit(MWindow *mwindow, AWindow *awindow, VWindow *vwindow);
	~LabelEdit();

	void run();
	void edit_label(Label *label);

// If it is being created or edited
	MWindow *mwindow;
	AWindow *awindow;
	VWindow *vwindow;

	Label *label;
};




class LabelEditWindow : public BC_Window
{
public:
	LabelEditWindow(MWindow *mwindow, LabelEdit *thread);
	~LabelEditWindow();

	void create_objects();


// Use this copy of the pointer in LabelEdit since multiple windows are possible	
	Label *label;
	MWindow *mwindow;
	LabelEdit *thread;
};



class LabelEditTitle : public BC_TextBox
{
public:
	LabelEditTitle(LabelEditWindow *window, int x, int y, int w);
	int handle_event();
	LabelEditWindow *window;
};


class LabelEditComments : public BC_TextBox
{
public:
	LabelEditComments(LabelEditWindow *window, int x, int y, int w, int rows);
	int handle_event();
	LabelEditWindow *window;
};






#endif

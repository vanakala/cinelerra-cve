#ifndef CLIPEDIT_H
#define CLIPEDIT_H

#include "awindow.inc"
#include "edl.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "thread.h"
#include "vwindow.inc"


class ClipEdit : public Thread
{
public:
	ClipEdit(MWindow *mwindow, AWindow *awindow, VWindow *vwindow);
	~ClipEdit();

	void run();
	void edit_clip(EDL *clip);
	void create_clip(EDL *clip);

// If it is being created or edited
	MWindow *mwindow;
	AWindow *awindow;
	VWindow *vwindow;


	EDL *clip;
	int create_it;
};




class ClipEditWindow : public BC_Window
{
public:
	ClipEditWindow(MWindow *mwindow, ClipEdit *thread);
	~ClipEditWindow();

	void create_objects();


// Use this copy of the pointer in ClipEdit since multiple windows are possible	
	EDL *clip;
	int create_it;
	MWindow *mwindow;
	ClipEdit *thread;
};



class ClipEditTitle : public BC_TextBox
{
public:
	ClipEditTitle(ClipEditWindow *window, int x, int y, int w);
	int handle_event();
	ClipEditWindow *window;
};


class ClipEditComments : public BC_TextBox
{
public:
	ClipEditComments(ClipEditWindow *window, int x, int y, int w, int rows);
	int handle_event();
	ClipEditWindow *window;
};






#endif

#ifndef SETVIDEO_H
#define SETVIDEO_H

#include "edl.inc"
#include "guicast.h"
#include "mwindow.inc"

class SetVideoThread;

class SetVideo : public BC_MenuItem
{
public:
	SetVideo(MWindow *mwindow);
	~SetVideo();
	
	int handle_event();
	MWindow *mwindow;
	SetVideoThread *thread;
};

class SetVideoWindow;

class SetVideoThread : public Thread
{
public:
	SetVideoThread(MWindow *mwindow);
	~SetVideoThread();

	void run();

	void update_window();
	void update_aspect();
	void load_params();
	void save_params();

	MWindow *mwindow;
	SetVideoWindow *window;
	int auto_aspect;
	int constrain_ratio;
	float aspect_w, aspect_h;
	double frame_rate;
	int color_model;
	int channels;
// Change a value to negative to signal it was the value changed.
// Want arrays for fast access.
	int orig_dimension[4];   // dimensions for getting scale
	int dimension[4];        // new dimensions to apply
	int offsets[4];          // Offsets of new images in old images
	float ratio[4];
};

class SetVideoWindow : public BC_Window
{
public:
	SetVideoWindow(MWindow *mwindow, SetVideoThread *thread);
	~SetVideoWindow();

	int create_objects();

	MWindow *mwindow;
	SetVideoThread *thread;
	BC_TextBox *dimension[4];
	BC_TextBox *ratio[4];
	BC_TextBox *aspect_w, *aspect_h;
};

class SetVChannels : public BC_TextBox
{
public:
	SetVChannels(SetVideoThread *thread, int x, int y);

	int handle_event();

	SetVideoThread *thread;
};

class ScaleConstrain : public BC_CheckBox
{
public:
	ScaleConstrain(int x, int y, SetVideoThread *thread);
	~ScaleConstrain();
	int handle_event();
	SetVideoThread *thread;
};

#endif

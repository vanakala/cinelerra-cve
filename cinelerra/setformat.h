#ifndef SETFORMAT_H
#define SETFORMAT_H


#include "edl.inc"
#include "guicast.h"
#include "mutex.inc"
#include "mwindow.inc"
#include "setformat.inc"
#include "thread.h"



class SetFormat : public BC_MenuItem
{
public:
	SetFormat(MWindow *mwindow);
	int handle_event();
	SetFormatThread *thread;
	MWindow *mwindow;
};


class SetFormatThread : public Thread
{
public:
	SetFormatThread(MWindow *mwindow);

	void run();

	void apply_changes();
	void update_window();
	void update_aspect();



	Mutex *window_lock;
	SetFormatWindow *window;
	MWindow *mwindow;
	EDL *new_settings;
	float ratio[2];
	int dimension[2];
	int orig_dimension[2];
	int auto_aspect;
	int constrain_ratio;
};


class SetSampleRateTextBox : public BC_TextBox
{
public:
	SetSampleRateTextBox(SetFormatThread *thread, int x, int y);
	int handle_event();
	SetFormatThread *thread;
};

class SetChannelsTextBox : public BC_TextBox
{
public:
	SetChannelsTextBox(SetFormatThread *thread, int x, int y);
	
	int handle_event();
	
	SetFormatThread *thread;
	MWindow *mwindow;
};


class SetChannelsCanvas : public BC_SubWindow
{
public:
	SetChannelsCanvas(MWindow *mwindow, 
		SetFormatThread *thread, 
		int x, 
		int y,
		int w,
		int h);
	~SetChannelsCanvas();
	
	int draw(int angle = -1);
	int get_dimensions(int channel_position, int &x, int &y, int &w, int &h);
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();

private:	
	int active_channel;   // for selection
	int degree_offset;
	int box_r;
	
	int poltoxy(int &x, int &y, int r, int d);
	int xytopol(int &d, int x, int y);
	MWindow *mwindow;
	SetFormatThread *thread;
	VFrame *temp_picon;
	RotateFrame *rotater;
};


class SetFrameRateTextBox : public BC_TextBox
{
public:
	SetFrameRateTextBox(SetFormatThread *thread, int x, int y);
	int handle_event();
	SetFormatThread *thread;
};

class ScaleSizeText : public BC_TextBox
{
public:
	ScaleSizeText(int x, int y, SetFormatThread *thread, int *output);
	~ScaleSizeText();
	int handle_event();
	SetFormatThread *thread;
	int *output;
};


class ScaleRatioText : public BC_TextBox
{
public:
	ScaleRatioText(int x, int y, SetFormatThread *thread, float *output);
	~ScaleRatioText();
	int handle_event();
	SetFormatThread *thread;
	float *output;
};

class ScaleAspectAuto : public BC_CheckBox
{
public:
	ScaleAspectAuto(int x, int y, SetFormatThread *thread);
	~ScaleAspectAuto();
	int handle_event();
	SetFormatThread *thread;
};

class ScaleAspectText : public BC_TextBox
{
public:
	ScaleAspectText(int x, int y, SetFormatThread *thread, float *output);
	~ScaleAspectText();
	int handle_event();
	SetFormatThread *thread;
	float *output;
};

class SetFormatApply : public BC_GenericButton
{
public:
	SetFormatApply(int x, int y, SetFormatThread *thread);
	int handle_event();
	SetFormatThread *thread;
};

class SetFormatWindow : public BC_Window
{
public:
	SetFormatWindow(MWindow *mwindow, SetFormatThread *thread);

	void create_objects();

	MWindow *mwindow;
	SetFormatThread *thread;
	SetChannelsCanvas *canvas;
	ScaleSizeText* dimension[2];
	ScaleRatioText* ratio[2];
	ScaleAspectText *aspect_w;
	ScaleAspectText *aspect_h;
};




#endif

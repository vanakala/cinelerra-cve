#ifndef CWINDOWTOOL_H
#define CWINDOWTOOL_H

#include "condition.inc"
#include "cwindowgui.inc"
#include "guicast.h"
#include "maskauto.inc"
#include "mwindow.inc"


class CWindowToolGUI;
class CWindowCoord;


// This common thread supports all the tool GUI's.
class CWindowTool : public Thread
{
public:
	CWindowTool(MWindow *mwindow, CWindowGUI *gui);
	~CWindowTool();
	
	void start_tool(int operation);
	void stop_tool();
	void run();
	void update_show_window();
	void update_values();

	MWindow *mwindow;
	CWindowGUI *gui;
	CWindowToolGUI *tool_gui;
	int done;
	int current_tool;
	Condition *input_lock;
	Condition *output_lock;
// Lock run and update_values
	Mutex *tool_gui_lock;
};

class CWindowToolGUI : public BC_Window
{
public:
	CWindowToolGUI(MWindow *mwindow, 
		CWindowTool *thread, 
		char *title,
		int w, 
		int h);
	~CWindowToolGUI();
	
	virtual void create_objects() {};
// Update the keyframe from text boxes
	virtual void handle_event() {};
// Update text boxes from keyframe here
	virtual void update() {};
// Update EDL and preview only
	virtual void update_preview() {};
	int current_operation;
	int close_event();
	int keypress_event();
	int translation_event();
	
	MWindow *mwindow;
	CWindowTool *thread;
	CWindowCoord *event_caller;
};

class CWindowCoord : public BC_TumbleTextBox
{
public:
	CWindowCoord(CWindowToolGUI *gui, int x, int y, float value);
	CWindowCoord(CWindowToolGUI *gui, int x, int y, int value);

// Calls the window's handle_event
	int handle_event();

	CWindowToolGUI *gui;
};

class CWindowCropOK : public BC_GenericButton
{
public:
	CWindowCropOK(MWindow *mwindow, CWindowToolGUI *gui, int x, int y);
// Perform the cropping operation
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowCropGUI : public CWindowToolGUI
{
public:
	CWindowCropGUI(MWindow *mwindow, CWindowTool *thread);
	~CWindowCropGUI();
	void create_objects();
	void update();
// Update the gui
	void handle_event();
	CWindowCoord *x1, *y1, *x2, *y2;
};

class CWindowMaskMode : public BC_PopupMenu
{
public:
	CWindowMaskMode(MWindow *mwindow, CWindowToolGUI *gui, int x, int y, char *text);
	void create_objects();
	int handle_event();
	static char* mode_to_text(int mode);
	int text_to_mode(char *text);
	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowMaskDelete : public BC_GenericButton
{
public:
	CWindowMaskDelete(MWindow *mwindow, CWindowToolGUI *gui, int x, int y);
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowMaskCycleNext : public BC_GenericButton
{
public:
	CWindowMaskCycleNext(MWindow *mwindow, CWindowToolGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowMaskCyclePrev : public BC_GenericButton
{
public:
	CWindowMaskCyclePrev(MWindow *mwindow, CWindowToolGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowMaskNumber : public BC_TumbleTextBox
{
public:
	CWindowMaskNumber(MWindow *mwindow, CWindowToolGUI *gui, int x, int y);
	~CWindowMaskNumber();
	int handle_event();
	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowMaskFeather : public BC_TumbleTextBox
{
public:
	CWindowMaskFeather(MWindow *mwindow, CWindowToolGUI *gui, int x, int y);
	~CWindowMaskFeather();
	int handle_event();
	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowMaskValue : public BC_ISlider
{
public:
	CWindowMaskValue(MWindow *mwindow, CWindowToolGUI *gui, int x, int y);
	~CWindowMaskValue();
	int handle_event();
	MWindow *mwindow;
	CWindowToolGUI *gui;
};


class CWindowMaskGUI : public CWindowToolGUI
{
public:
	CWindowMaskGUI(MWindow *mwindow, CWindowTool *thread);
	~CWindowMaskGUI();
	void create_objects();
	void update();
	void handle_event();
	void get_keyframe(Track* &track, 
		MaskAuto* &keyframe, 
		SubMask* &mask, 
		MaskPoint* &point,
		int create_it);
	void update_preview();

	CWindowCoord *x, *y;
	CWindowMaskMode *mode;
	CWindowMaskFeather *feather;
	CWindowMaskDelete *delete_point;
// Not necessary if all keyframes have same points
//	CWindowMaskCycleNext *next_point;
//	CWindowMaskCyclePrev *prev_point;
	CWindowMaskNumber *number;
	CWindowMaskValue *value;
};



class CWindowCameraGUI : public CWindowToolGUI
{
public:
	CWindowCameraGUI(MWindow *mwindow, CWindowTool *thread);
	~CWindowCameraGUI();
	void create_objects();
	void update();
	void update_preview();

// Update the keyframe from text boxes
	void handle_event();
	BezierAuto* get_keyframe();
	CWindowCoord *x, *y, *z;
};

class CWindowCameraLeft : public BC_Button
{
public:
	CWindowCameraLeft(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowCameraGUI *gui;
};

class CWindowCameraCenter : public BC_Button
{
public:
	CWindowCameraCenter(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowCameraGUI *gui;
};

class CWindowCameraRight : public BC_Button
{
public:
	CWindowCameraRight(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowCameraGUI *gui;
};

class CWindowCameraTop : public BC_Button
{
public:
	CWindowCameraTop(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowCameraGUI *gui;
};

class CWindowCameraMiddle : public BC_Button
{
public:
	CWindowCameraMiddle(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowCameraGUI *gui;
};

class CWindowCameraBottom : public BC_Button
{
public:
	CWindowCameraBottom(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowCameraGUI *gui;
};

class CWindowProjectorGUI : public CWindowToolGUI
{
public:
	CWindowProjectorGUI(MWindow *mwindow, CWindowTool *thread);
	~CWindowProjectorGUI();
	void create_objects();
	void update();
	void update_preview();
	void handle_event();
	BezierAuto* get_keyframe();
	CWindowCoord *x, *y, *z;
};

class CWindowProjectorLeft : public BC_Button
{
public:
	CWindowProjectorLeft(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowProjectorGUI *gui;
};

class CWindowProjectorCenter : public BC_Button
{
public:
	CWindowProjectorCenter(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowProjectorGUI *gui;
};

class CWindowProjectorRight : public BC_Button
{
public:
	CWindowProjectorRight(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowProjectorGUI *gui;
};

class CWindowProjectorTop : public BC_Button
{
public:
	CWindowProjectorTop(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowProjectorGUI *gui;
};

class CWindowProjectorMiddle : public BC_Button
{
public:
	CWindowProjectorMiddle(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowProjectorGUI *gui;
};

class CWindowProjectorBottom : public BC_Button
{
public:
	CWindowProjectorBottom(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowProjectorGUI *gui;
};



#endif

// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef CWINDOWTOOL_H
#define CWINDOWTOOL_H

#include "condition.inc"
#include "cropauto.inc"
#include "cwindowgui.inc"
#include "floatauto.inc"
#include "maskauto.inc"

class CWindowToolGUI;
class CWindowCoord;
class CWindowCropBeforePlugins;
class CWindowMaskGUI;

struct tool_names
{
	const char *text;
	int value;
};

// This common thread supports all the tool GUI's.
class CWindowTool : public Thread
{
public:
	CWindowTool(CWindowGUI *gui);
	~CWindowTool();

// Called depending on state of toggle button
	void start_tool(int operation);
	void stop_tool();

// Called when window is visible
	void show_tool();
	void hide_tool();

	void run();
	int update_show_window();
	void raise_window();
	void update_values();

	CWindowGUI *cwindowgui;
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
	CWindowToolGUI(CWindowTool *thread,
		const char *title,
		int w, 
		int h);

// Update the keyframe from text boxes
	virtual int handle_event() { return 0; };
// Update text boxes from keyframe here
	virtual void update() {};
// Update EDL and preview only
	virtual void update_preview() {};
// Allocate automation
	virtual void allocate_autos() {};

	void close_event();
	void translation_event();

	int current_operation;
	CWindowTool *thread;
	CWindowCoord *event_caller;

protected:
	void get_keyframes(FloatAuto* &x_auto, FloatAuto* &y_auto, FloatAuto* &z_auto,
		int camera, int create_x, int create_y, int create_z);

	FloatAuto *local_x;
	FloatAuto *local_y;
	FloatAuto *local_z;
};

class CWindowCoord : public BC_TumbleTextBox
{
public:
	CWindowCoord(CWindowToolGUI *gui, int x, int y, double value, int logincrement);
	CWindowCoord(int x, int y, double value, CWindowToolGUI *gui);
	CWindowCoord(CWindowToolGUI *gui, int x, int y, int value);

// Calls the window's handle_event
	int handle_event();

	CWindowToolGUI *gui;
};


class CWindowCropGUI : public CWindowToolGUI
{
public:
	CWindowCropGUI(CWindowTool *thread);

	void update();
// Update the gui
	int handle_event();
	CropAuto *get_keyframe(int create_it);
	void allocate_autos();

	CWindowCoord *x1, *y1, *width, *height;
	CWindowCropBeforePlugins *apply;

	double top;
	double left;
	double right;
	double bottom;
	int before_plugins;
};

class CWindowCropBeforePlugins : public BC_CheckBox
{
public:
	CWindowCropBeforePlugins(CWindowCropGUI *gui, int x, int y);

	int handle_event();

	CWindowCropGUI *gui;
};


class CWindowMaskMode : public BC_PopupMenu
{
public:
	CWindowMaskMode(CWindowMaskGUI *gui, int x, int y);

	int handle_event();
	static const char *name(int mode);
	static int mode(const char *text);

private:
	static struct tool_names modenames[];
	CWindowMaskGUI *gui;
};

class CWindowMaskDelete : public BC_GenericButton
{
public:
	CWindowMaskDelete(CWindowToolGUI *gui, int x, int y);

	int handle_event();
	int keypress_event();

	CWindowToolGUI *gui;
};

class CWindowMaskNumber : public BC_TumbleTextBox
{
public:
	CWindowMaskNumber(CWindowToolGUI *gui, int x, int y);

	int handle_event();

	CWindowToolGUI *gui;
};

class CWindowMaskFeather : public BC_TumbleTextBox
{
public:
	CWindowMaskFeather(CWindowToolGUI *gui, int x, int y);

	int handle_event();

	CWindowToolGUI *gui;
};

class CWindowMaskValue : public BC_ISlider
{
public:
	CWindowMaskValue(CWindowToolGUI *gui, int x, int y);

	int handle_event();

	CWindowToolGUI *gui;
};

class CWindowMaskBeforePlugins : public BC_CheckBox
{
public:
	CWindowMaskBeforePlugins(CWindowToolGUI *gui, int x, int y);

	int handle_event();

	CWindowToolGUI *gui;
};


class CWindowMaskGUI : public CWindowToolGUI
{
public:
	CWindowMaskGUI(CWindowTool *thread);
	~CWindowMaskGUI();

	void update();
	int handle_event();
	void get_keyframe(Track* &track, MaskAuto* &keyframe,
		SubMask* &mask, MaskPoint* &point, int create_it);
	void update_preview();
	void allocate_autos();

	CWindowCoord *x, *y;
	CWindowMaskMode *mode;
	CWindowMaskFeather *feather;
	CWindowMaskDelete *delete_point;
	CWindowMaskNumber *number;
	CWindowMaskValue *value;
	CWindowMaskBeforePlugins *apply_before_plugins;

private:
	MaskAuto *localauto;
};


class CWindowEyedropGUI : public CWindowToolGUI
{
public:
	CWindowEyedropGUI(CWindowTool *thread);

	void update();

	BC_Title *red, *green, *blue;
	BC_SubWindow *sample;
};


// Configuration for all possible Keyframe Tangent Mode toggles
struct _TGD
{
	tgnt_mode mode;
	bool use_camera;
	const char* icon_id;
	const char* tooltip;
};

// Implementation Class for Keyframe Tangent Mode buttons
//
// This button reflects the state of the "current" keyframe
// (the nearest keyframe on the left) for all three automation
// lines together. Clicking on this button (re)sets the tangent
// mode for the three "current" keyframes simultanously, but
// never creates a new keyframe.
//
class CWindowTangentToggle : public BC_Toggle
{
public:
	CWindowTangentToggle(_TGD mode, CWindowToolGUI *gui,
		int x, int y);

	void check_toggle_state(FloatAuto *x, FloatAuto *y, FloatAuto *z);
	int handle_event();

private:
	_TGD cfg;
	CWindowToolGUI *gui;
};


class CWindowCamProjGUI : public CWindowToolGUI
{
public:
	CWindowCamProjGUI(CWindowTool *thread,
		const char *tooltitle, int camera);
	~CWindowCamProjGUI();

	void update();
	void update_preview();

// Update the keyframe from text boxes
	int handle_event();

	int is_camera;
	CWindowCoord *x, *y, *z;

private:
// Toggles for keyframe tangent mode (projector automation only)
	CWindowTangentToggle *t_smooth;
	CWindowTangentToggle *t_linear;
};


class CWindowCameraGUI : public CWindowCamProjGUI
{
public:
	CWindowCameraGUI(CWindowTool *thread);
};

class CWindowProjectorGUI : public CWindowCamProjGUI
{
public:
	CWindowProjectorGUI(CWindowTool *thread);
};

class CWindowCPLeft : public BC_Button
{
public:
	CWindowCPLeft(CWindowCamProjGUI *gui, int x, int y);

	int handle_event();

	CWindowCamProjGUI *gui;
};

class CWindowCPCenter : public BC_Button
{
public:
	CWindowCPCenter(CWindowCamProjGUI *gui, int x, int y);

	int handle_event();

	CWindowCamProjGUI *gui;
};

class CWindowCPRight : public BC_Button
{
public:
	CWindowCPRight(CWindowCamProjGUI *gui, int x, int y);

	int handle_event();

	CWindowCamProjGUI *gui;
};

class CWindowCPTop : public BC_Button
{
public:
	CWindowCPTop(CWindowCamProjGUI *gui, int x, int y);

	int handle_event();

	CWindowCamProjGUI *gui;
};

class CWindowCPMiddle : public BC_Button
{
public:
	CWindowCPMiddle(CWindowCamProjGUI *gui, int x, int y);

	int handle_event();

	CWindowCamProjGUI *gui;
};

class CWindowCPBottom : public BC_Button
{
public:
	CWindowCPBottom(CWindowCamProjGUI *gui, int x, int y);

	int handle_event();

	CWindowCamProjGUI *gui;
};

class CWindowRulerGUI : public CWindowToolGUI
{
public:
	CWindowRulerGUI(CWindowTool *thread);

	void update();

	BC_Title *current;
	BC_Title *point1;
	BC_Title *point2;
	BC_Title *distance;
	BC_Title *angle;
};

#endif

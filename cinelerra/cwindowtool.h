
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef CWINDOWTOOL_H
#define CWINDOWTOOL_H

#include "condition.inc"
#include "cwindowgui.inc"
#include "guicast.h"
#include "maskauto.inc"
#include "mwindow.inc"

class CWindowToolGUI;
class CWindowCoord;
class CWindowTangentToggle;

// This common thread supports all the tool GUI's.
class CWindowTool : public Thread
{
public:
	CWindowTool(MWindow *mwindow, CWindowGUI *gui);
	~CWindowTool();

// Called depending on state of toggle button
	void start_tool(int operation);
	void stop_tool();

// Called when window is visible
	void show_tool();
	void hide_tool();

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
		const char *title,
		int w, 
		int h);

// Update the keyframe from text boxes
	virtual int handle_event() {};
// Update text boxes from keyframe here
	virtual void update() {};
// Update EDL and preview only
	virtual void update_preview() {};

	void close_event();
	void translation_event();

	int current_operation;
	MWindow *mwindow;
	CWindowTool *thread;
	CWindowCoord *event_caller;
};

class CWindowCoord : public BC_TumbleTextBox
{
public:
	CWindowCoord(CWindowToolGUI *gui, int x, int y, float value, int logincrement);
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

	void update();
// Update the gui
	int handle_event();
	CWindowCoord *x1, *y1, *width, *height;
};

class CWindowMaskMode : public BC_PopupMenu
{
public:
	CWindowMaskMode(MWindow *mwindow, CWindowToolGUI *gui, int x, int y, const char *text);

	void create_objects();
	int handle_event();
	static const char* mode_to_text(int mode);
	int text_to_mode(const char *text);

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

class CWindowMaskNumber : public BC_TumbleTextBox
{
public:
	CWindowMaskNumber(MWindow *mwindow, CWindowToolGUI *gui, int x, int y);

	int handle_event();

	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowMaskFeather : public BC_TumbleTextBox
{
public:
	CWindowMaskFeather(MWindow *mwindow, CWindowToolGUI *gui, int x, int y);

	int handle_event();

	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowMaskValue : public BC_ISlider
{
public:
	CWindowMaskValue(MWindow *mwindow, CWindowToolGUI *gui, int x, int y);

	int handle_event();
	MWindow *mwindow;
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
	CWindowMaskGUI(MWindow *mwindow, CWindowTool *thread);
	~CWindowMaskGUI();

	void update();
	int handle_event();
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
	CWindowMaskNumber *number;
	CWindowMaskValue *value;
	CWindowMaskBeforePlugins *apply_before_plugins;
};


class CWindowEyedropGUI : public CWindowToolGUI
{
public:
	CWindowEyedropGUI(MWindow *mwindow, CWindowTool *thread);

	void update();

	BC_Title *red, *green, *blue;
	BC_SubWindow *sample;
};


class CWindowCameraGUI : public CWindowToolGUI
{
public:
	CWindowCameraGUI(MWindow *mwindow, CWindowTool *thread);

	void update();
	void update_preview();

// Update the keyframe from text boxes
	int handle_event();

	CWindowCoord *x, *y, *z;

private:
// Toggles for keyframe tangent mode (projector automation only)
	CWindowTangentToggle *t_smooth;
	CWindowTangentToggle *t_linear;
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

	void update();
	void update_preview();
	int handle_event();

	CWindowCoord *x, *y, *z;
private:
// Toggles for keyframe tangent mode (projector automation only)
	CWindowTangentToggle *t_smooth;
	CWindowTangentToggle *t_linear;
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

#ifndef CPANEL_H
#define CPANEL_H

#include "cwindowgui.inc"
#include "edl.inc"
#include "guicast.h"
#include "mwindow.inc"

class CPanel
{
public:
	CPanel(MWindow *mwindow, CWindowGUI *subwindow, int x, int y, int w, int h);
	~CPanel();

	int create_objects();
	void reposition_buttons(int x, int y);
	void set_operation(int value);

	MWindow *mwindow;
	CWindowGUI *subwindow;

	int x, y, w, h;

	BC_Toggle* operation[CPANEL_OPERATIONS];
};

class CPanelMask : public BC_Toggle
{
public:
	CPanelMask(MWindow *mwindow, CPanel *gui, int x, int y);
	~CPanelMask();
	int handle_event();
	MWindow *mwindow;
	CPanel *gui;
};

class CPanelTitleSafe : public BC_Toggle
{
public:
	CPanelTitleSafe(MWindow *mwindow, CPanel *gui, int x, int y);
	~CPanelTitleSafe();
	int handle_event();
	MWindow *mwindow;
	CPanel *gui;
};

class CPanelErase : public BC_Toggle
{
public:
	CPanelErase(MWindow *mwindow, CPanel *gui, int x, int y);
	~CPanelErase();
	int handle_event();
	MWindow *mwindow;
	CPanel *gui;
};

class CPanelAntierase : public BC_Toggle
{
public:
	CPanelAntierase(MWindow *mwindow, CPanel *gui, int x, int y);
	~CPanelAntierase();
	int handle_event();
	MWindow *mwindow;
	CPanel *gui;
};

class CPanelProtect : public BC_Toggle
{
public:
	CPanelProtect(MWindow *mwindow, CPanel *gui, int x, int y);
	~CPanelProtect();
	int handle_event();
	MWindow *mwindow;
	CPanel *gui;
};

class CPanelMagnify : public BC_Toggle
{
public:
	CPanelMagnify(MWindow *mwindow, CPanel *gui, int x, int y);
	~CPanelMagnify();
	int handle_event();
	MWindow *mwindow;
	CPanel *gui;
};

class CPanelCamera : public BC_Toggle
{
public:
	CPanelCamera(MWindow *mwindow, CPanel *gui, int x, int y);
	~CPanelCamera();
	int handle_event();
	MWindow *mwindow;
	CPanel *gui;
};

class CPanelProj : public BC_Toggle
{
public:
	CPanelProj(MWindow *mwindow, CPanel *gui, int x, int y);
	~CPanelProj();
	int handle_event();
	MWindow *mwindow;
	CPanel *gui;
};

class CPanelCrop : public BC_Toggle
{
public:
	CPanelCrop(MWindow *mwindow, CPanel *gui, int x, int y);
	~CPanelCrop();
	int handle_event();
	MWindow *mwindow;
	CPanel *gui;
};

class CPanelToolWindow : public BC_Toggle
{
public:
	CPanelToolWindow(MWindow *mwindow, CPanel *gui, int x, int y);
	~CPanelToolWindow();
	int handle_event();
	MWindow *mwindow;
	CPanel *gui;
};



#endif

// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef CPANEL_H
#define CPANEL_H

#include "bctoggle.h"
#include "cwindowgui.inc"
#include "edl.inc"

class CPanel
{
public:
	CPanel(CWindowGUI *subwindow, int x, int y, int w, int h);

	void reposition_buttons(int x, int y);
	void set_operation(int value);

	CWindowGUI *subwindow;
private:
	BC_Toggle* operation[CPANEL_OPERATIONS];
};

class CPanelMask : public BC_Toggle
{
public:
	CPanelMask(CPanel *gui, int x, int y);

	int handle_event();
private:
	CPanel *gui;
};

class CPanelRuler : public BC_Toggle
{
public:
	CPanelRuler(CPanel *gui, int x, int y);

	int handle_event();
private:
	CPanel *gui;
};

class CPanelTitleSafe : public BC_Toggle
{
public:
	CPanelTitleSafe(CPanel *gui, int x, int y);

	int handle_event();
private:
	CPanel *gui;
};

class CPanelProtect : public BC_Toggle
{
public:
	CPanelProtect(CPanel *gui, int x, int y);

	int handle_event();
private:
	CPanel *gui;
};

class CPanelMagnify : public BC_Toggle
{
public:
	CPanelMagnify(CPanel *gui, int x, int y);

	int handle_event();
private:
	CPanel *gui;
};

class CPanelCamera : public BC_Toggle
{
public:
	CPanelCamera(CPanel *gui, int x, int y);

	int handle_event();
private:
	CPanel *gui;
};

class CPanelProj : public BC_Toggle
{
public:
	CPanelProj(CPanel *gui, int x, int y);

	int handle_event();
private:
	CPanel *gui;
};

class CPanelCrop : public BC_Toggle
{
public:
	CPanelCrop(CPanel *gui, int x, int y);

	int handle_event();
private:
	CPanel *gui;
};

class CPanelEyedrop : public BC_Toggle
{
public:
	CPanelEyedrop(CPanel *gui, int x, int y);

	int handle_event();
private:
	CPanel *gui;
};

class CPanelToolWindow : public BC_Toggle
{
public:
	CPanelToolWindow(CPanel *gui, int x, int y);

	int handle_event();
private:
	CPanel *gui;
};

#endif

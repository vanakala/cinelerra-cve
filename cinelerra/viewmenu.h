#ifndef VIEWMENU_H
#define VIEWMENU_H

#include "guicast.h"
#include "mainmenu.inc"
#include "mwindow.inc"

class CameraAutomation : public BC_MenuItem
{
public:
	CameraAutomation(MWindow *mwindow, char *hotkey);
	int handle_event();
	MWindow *mwindow;
};

class FadeAutomation : public BC_MenuItem
{
public:
	FadeAutomation(MWindow *mwindow, char *hotkey);
	int handle_event();
	MWindow *mwindow;
};

class MuteAutomation : public BC_MenuItem
{
public:
	MuteAutomation(MWindow *mwindow, char *hotkey);
	int handle_event();
	MWindow *mwindow;
};

class PanAutomation : public BC_MenuItem
{
public:
	PanAutomation(MWindow *mwindow, char *hotkey);
	int handle_event();
	int change_channels(int old_channels, int new_channels);
	MWindow *mwindow;
};

/*
 * class PlayAutomation : public BC_MenuItem
 * {
 * public:
 * 	PlayAutomation(MWindow *mwindow, char *hotkey);
 * 	int handle_event();
 * 	MWindow *mwindow;
 * };
 */

class ProjectAutomation : public BC_MenuItem
{
public:
	ProjectAutomation(MWindow *mwindow, char *hotkey);
	int handle_event();
	MWindow *mwindow;
};

class ShowEdits : public BC_MenuItem
{
public:
	ShowEdits(MWindow *mwindow, char *hotkey);
	int handle_event();
	MWindow *mwindow;
};

class ShowKeyframes : public BC_MenuItem
{
public:
	ShowKeyframes(MWindow *mwindow, char *hotkey);
	int handle_event();
	MWindow *mwindow;
};

class ShowRenderedOutput : public BC_MenuItem
{
public:
	ShowRenderedOutput(MWindow *mwindow, char *hotkey);
	int handle_event();
	MWindow *mwindow;
};

class ShowTitles : public BC_MenuItem
{
public:
	ShowTitles(MWindow *mwindow, char *hotkey);
	int handle_event();
	MWindow *mwindow;
};

class ShowTransitions : public BC_MenuItem
{
public:
	ShowTransitions(MWindow *mwindow, char *hotkey);
	int handle_event();
	MWindow *mwindow;
};

class PluginAutomation : public BC_MenuItem
{
public:
	PluginAutomation(MWindow *mwindow, char *hotkey);
	int handle_event();
	MWindow *mwindow;
};

class ModeAutomation : public BC_MenuItem
{
public:
	ModeAutomation(MWindow *mwindow, char *hotkey);
	int handle_event();
	MWindow *mwindow;
};

class MaskAutomation : public BC_MenuItem
{
public:
    MaskAutomation(MWindow *mwindow, char *hotkey);
	int handle_event();
	MWindow *mwindow;
};

class CZoomAutomation : public BC_MenuItem
{
public:
    CZoomAutomation(MWindow *mwindow, char *hotkey);
	int handle_event();
	MWindow *mwindow;
};

class PZoomAutomation : public BC_MenuItem
{
public:
    PZoomAutomation(MWindow *mwindow, char *hotkey);
	int handle_event();
	MWindow *mwindow;
};

#endif

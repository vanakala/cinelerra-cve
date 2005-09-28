#ifndef VIEWMENU_H
#define VIEWMENU_H

#include "guicast.h"
#include "mainmenu.inc"
#include "mwindow.inc"

class ShowAssets : public BC_MenuItem
{
public:
	ShowAssets(MWindow *mwindow, char *hotkey);
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

class ShowAutomation : public BC_MenuItem
{
public:
	ShowAutomation(MWindow *mwindow, 
		char *text,
		char *hotkey,
		int subscript);
	int handle_event();
	void update_toggle();
	MWindow *mwindow;
	int subscript;
};


#endif

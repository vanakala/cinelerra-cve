#ifndef APANEL_H
#define APANEL_H


#include "cwindowgui.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "track.inc"



class APanelPluginAuto;
class APanelMute;
class APanelPlay;

class APanel
{
public:
	APanel(MWindow *mwindow, CWindowGUI *subwindow, int x, int y, int w, int h);
	~APanel();

	int create_objects();

	MWindow *mwindow;
	CWindowGUI *subwindow;
	int x, y, w, h;
	APanelPluginAuto *plugin_autos[PLUGINS];
	APanelMute *mute;
	APanelPlay *play;
};

class APanelPluginAuto : public BC_FPot
{
public:
	APanelPluginAuto(MWindow *mwindow, APanel *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	APanel *gui;
};


class APanelMute : public BC_CheckBox
{
public:
	APanelMute(MWindow *mwindow, APanel *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	APanel *gui;
};


class APanelPlay : public BC_CheckBox
{
public:
	APanelPlay(MWindow *mwindow, APanel *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	APanel *gui;
};


#endif

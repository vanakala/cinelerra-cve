#ifndef MBUTTONS_H
#define MBUTTONS_H

class KeyFrameButton;
class ExpandX;
class ZoomX;
class ExpandY;
class ZoomY;
class ExpandTrack;
class ZoomTrack;
class ExpandVideo;
class MainEditing;
class ZoomVideo;
class LabelButton;
class Cut;
class Copy;
class Paste;

#include "editpanel.h"
#include "guicast.h"
#include "labelnavigate.inc"
#include "mbuttons.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "playtransport.h"
#include "record.inc"




class MButtons : public BC_SubWindow
{
public:
	MButtons(MWindow *mwindow, MWindowGUI *gui);
	~MButtons();

	int create_objects();
	int resize_event();
	int keypress_event();
	void update();

	MWindowGUI *gui;
	MWindow *mwindow;
	PlayTransport *transport;
	MainEditing *edit_panel;


};

class MainTransport : public PlayTransport
{
public:
	MainTransport(MWindow *mwindow, MButtons *mbuttons, int x, int y);
	void goto_start();
	void goto_end();
};

class MainEditing : public EditPanel
{
public:
	MainEditing(MWindow *mwindow, MButtons *mbuttons, int x, int y);

	
	MWindow *mwindow;
	MButtons *mbuttons;
};

#endif

#ifndef LEVELWINDOWGUI_H
#define LEVELWINDOWGUI_H

class LevelWindowReset;

#include "guicast.h"
#include "levelwindow.inc"
#include "maxchannels.h"
#include "meterpanel.inc"
#include "mwindow.inc"

class LevelWindowGUI : public BC_Window
{
public:
	LevelWindowGUI(MWindow *mwindow, LevelWindow *thread);
	~LevelWindowGUI();

	int create_objects();
	int resize_event(int w, int h);
	int translation_event();
	int close_event();
	int reset_over();
	int keypress_event();

	MWindow *mwindow;

	MeterPanel *panel;
	LevelWindow *thread;
};


#endif

#ifndef MAINCURSOR_H
#define MAINCURSOR_H

#include "guicast.h"
#include "mwindow.inc"
#include "mwindowgui.inc"

class MainCursor
{
public:
	MainCursor(MWindow *mwindow, MWindowGUI *gui);
	~MainCursor();

	void create_objects();
	int repeat_event(long duration);
	void draw();
	void hide();
	void flash();
	void activate();
	void deactivate();
	void show();
	void restore();
	void update();

	MWindow *mwindow;
	MWindowGUI *gui;
	int visible;
	double selectionstart, selectionend;
	long zoom_sample;
	double view_start;
	long pixel2, pixel1;
	int active;
	int playing_back;
};

#endif

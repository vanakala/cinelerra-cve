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
	int repeat_event(int64_t duration);
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
	int64_t zoom_sample;
	double view_start;
	int64_t pixel2, pixel1;
	int active;
	int playing_back;
};

#endif

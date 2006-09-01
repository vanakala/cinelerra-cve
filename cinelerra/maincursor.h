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
	void draw(int flash);
	void hide(int do_plugintoggles = 1);
	void flash();
	void activate();
	void deactivate();
	void show(int do_plugintoggles = 1);
	void restore(int do_plugintoggles);
	void update();
	void focus_in_event();
	void focus_out_event();

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

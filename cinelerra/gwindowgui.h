#ifndef GWINDOWGUI_H
#define GWINDOWGUI_H

#include "automation.inc"
#include "guicast.h"
#include "mwindow.inc"

class GWindowToggle;

#define ASSETS 0
#define TITLES 1
#define TRANSITIONS 2
#define PLUGIN_AUTOS 3
#define OTHER_TOGGLES 4

class GWindowGUI : public BC_Window
{
public:
	GWindowGUI(MWindow *mwindow, int w, int h);
	static void calculate_extents(BC_WindowBase *gui, int *w, int *h);
	void create_objects();
	int translation_event();
	int close_event();
	int keypress_event();
	void update_toggles(int use_lock);
	void update_mwindow();

	MWindow *mwindow;
	GWindowToggle *other[OTHER_TOGGLES];
	GWindowToggle *auto_toggle[AUTOMATION_TOTAL];
};

class GWindowToggle : public BC_CheckBox
{
public:
	GWindowToggle(MWindow *mwindow, 
		GWindowGUI *gui, 
		int x, 
		int y, 
		int subscript,
		int other, 
		char *text);
	int handle_event();
	void update();

	static int* get_main_value(MWindow *mwindow, int subscript, int other);

	MWindow *mwindow;
	GWindowGUI *gui;
	int subscript;
	int other;
};

#endif

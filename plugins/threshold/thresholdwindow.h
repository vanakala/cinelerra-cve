#ifndef THRESHOLDWINDOW_H
#define THRESHOLDWINDOW_H

#include "guicast.h"
#include "pluginvclient.h"
#include "threshold.inc"
#include "thresholdwindow.inc"

class ThresholdMin : public BC_TumbleTextBox
{
public:
	ThresholdMin(ThresholdMain *plugin,
		ThresholdWindow *gui,
		int x,
		int y,
		int w);
	int handle_event();
	ThresholdMain *plugin;
	ThresholdWindow *gui;
};

class ThresholdMax : public BC_TumbleTextBox
{
public:
	ThresholdMax(ThresholdMain *plugin,
		ThresholdWindow *gui,
		int x,
		int y,
		int w);
	int handle_event();
	ThresholdMain *plugin;
	ThresholdWindow *gui;
};

class ThresholdPlot : public BC_CheckBox
{
public:
	ThresholdPlot(ThresholdMain *plugin,
		int x,
		int y);
	int handle_event();
	ThresholdMain *plugin;
};

class ThresholdCanvas : public BC_SubWindow
{
public:
	ThresholdCanvas(ThresholdMain *plugin,
		ThresholdWindow *gui,
		int x,
		int y,
		int w,
		int h);
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	void draw();

	ThresholdMain *plugin;
	ThresholdWindow *gui;
	int state;
	enum
	{
		NO_OPERATION,
		DRAG_SELECTION
	};
	int x1;
	int x2;
	int center_x;
};

class ThresholdWindow : public BC_Window
{
public:
	ThresholdWindow(ThresholdMain *plugin, int x, int y);
	~ThresholdWindow();
	
	int create_objects();
	int close_event();

	ThresholdMain *plugin;
	ThresholdMin *min;
	ThresholdMax *max;
	ThresholdCanvas *canvas;
	ThresholdPlot *plot;
};

PLUGIN_THREAD_HEADER(ThresholdMain, ThresholdThread, ThresholdWindow)


#endif







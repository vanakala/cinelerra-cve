#ifndef LINEARIZEWINDOW_H
#define LINEARIZEWINDOW_H


class GammaThread;
class GammaWindow;
class MaxSlider;
class MaxText;
class GammaSlider;
class GammaText;
class GammaAuto;
class GammaPlot;
class GammaColorPicker;

#include "filexml.h"
#include "guicast.h"
#include "mutex.h"
#include "gamma.h"
#include "pluginclient.h"


PLUGIN_THREAD_HEADER(GammaMain, GammaThread, GammaWindow)

class GammaWindow : public BC_Window
{
public:
	GammaWindow(GammaMain *client, int x, int y);

	int create_objects();
	int close_event();
	void update();
	void update_histogram();


	BC_SubWindow *histogram;
	GammaMain *client;
	MaxSlider *max_slider;
	MaxText *max_text;
	GammaSlider *gamma_slider;
	GammaText *gamma_text;
	GammaAuto *automatic;
	GammaPlot *plot;
};

class MaxSlider : public BC_FSlider
{
public:
	MaxSlider(GammaMain *client, 
		GammaWindow *gui, 
		int x, 
		int y,
		int w);
	int handle_event();
	GammaMain *client;
	GammaWindow *gui;
};

class MaxText : public BC_TextBox
{
public:
	MaxText(GammaMain *client,
		GammaWindow *gui,
		int x,
		int y,
		int w);
	int handle_event();
	GammaMain *client;
	GammaWindow *gui;
};

class GammaSlider : public BC_FSlider
{
public:
	GammaSlider(GammaMain *client, 
		GammaWindow *gui, 
		int x, 
		int y,
		int w);
	int handle_event();
	GammaMain *client;
	GammaWindow *gui;
};

class GammaText : public BC_TextBox
{
public:
	GammaText(GammaMain *client,
		GammaWindow *gui,
		int x,
		int y,
		int w);
	int handle_event();
	GammaMain *client;
	GammaWindow *gui;
};

class GammaAuto : public BC_CheckBox
{
public:
	GammaAuto(GammaMain *plugin, int x, int y);
	int handle_event();
	GammaMain *plugin;
};

class GammaPlot : public BC_CheckBox
{
public:
	GammaPlot(GammaMain *plugin, int x, int y);
	int handle_event();
	GammaMain *plugin;
};

class GammaColorPicker : public BC_GenericButton
{
public:
	GammaColorPicker(GammaMain *plugin, 
		GammaWindow *gui, 
		int x, 
		int y);
	int handle_event();
	GammaMain *plugin;
	GammaWindow *gui;
};

#endif

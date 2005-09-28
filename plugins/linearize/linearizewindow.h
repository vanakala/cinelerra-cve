#ifndef LINEARIZEWINDOW_H
#define LINEARIZEWINDOW_H


class LinearizeThread;
class LinearizeWindow;
class MaxSlider;
class MaxText;
class GammaSlider;
class GammaText;
class LinearizeAuto;
class LinearizeColorPicker;

#include "filexml.h"
#include "guicast.h"
#include "mutex.h"
#include "linearize.h"
#include "pluginclient.h"


PLUGIN_THREAD_HEADER(LinearizeMain, LinearizeThread, LinearizeWindow)

class LinearizeWindow : public BC_Window
{
public:
	LinearizeWindow(LinearizeMain *client, int x, int y);

	int create_objects();
	int close_event();
	void update();
	void update_histogram();


	BC_SubWindow *histogram;
	LinearizeMain *client;
	MaxSlider *max_slider;
	MaxText *max_text;
	GammaSlider *gamma_slider;
	GammaText *gamma_text;
	LinearizeAuto *automatic;
};

class MaxSlider : public BC_FSlider
{
public:
	MaxSlider(LinearizeMain *client, 
		LinearizeWindow *gui, 
		int x, 
		int y,
		int w);
	int handle_event();
	LinearizeMain *client;
	LinearizeWindow *gui;
};

class MaxText : public BC_TextBox
{
public:
	MaxText(LinearizeMain *client,
		LinearizeWindow *gui,
		int x,
		int y,
		int w);
	int handle_event();
	LinearizeMain *client;
	LinearizeWindow *gui;
};

class GammaSlider : public BC_FSlider
{
public:
	GammaSlider(LinearizeMain *client, 
		LinearizeWindow *gui, 
		int x, 
		int y,
		int w);
	int handle_event();
	LinearizeMain *client;
	LinearizeWindow *gui;
};

class GammaText : public BC_TextBox
{
public:
	GammaText(LinearizeMain *client,
		LinearizeWindow *gui,
		int x,
		int y,
		int w);
	int handle_event();
	LinearizeMain *client;
	LinearizeWindow *gui;
};

class LinearizeAuto : public BC_CheckBox
{
public:
	LinearizeAuto(LinearizeMain *plugin, int x, int y);
	int handle_event();
	LinearizeMain *plugin;
};

class LinearizeColorPicker : public BC_GenericButton
{
public:
	LinearizeColorPicker(LinearizeMain *plugin, 
		LinearizeWindow *gui, 
		int x, 
		int y);
	int handle_event();
	LinearizeMain *plugin;
	LinearizeWindow *gui;
};

#endif

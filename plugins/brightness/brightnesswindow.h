#ifndef BRIGHTNESSWINDOW_H
#define BRIGHTNESSWINDOW_H


class BrightnessThread;
class BrightnessWindow;
class BrightnessSlider;
class BrightnessLuma;

#include "brightness.h"
#include "guicast.h"
#include "mutex.h"
#include "pluginvclient.h"
#include "thread.h"

PLUGIN_THREAD_HEADER(BrightnessMain, BrightnessThread, BrightnessWindow)

class BrightnessWindow : public BC_Window
{
public:
	BrightnessWindow(BrightnessMain *client, int x, int y);
	~BrightnessWindow();

	int create_objects();
	int close_event();

	BrightnessMain *client;
	BrightnessSlider *brightness;
	BrightnessSlider *contrast;
	BrightnessLuma *luma;
};

class BrightnessSlider : public BC_FSlider
{
public:
	BrightnessSlider(BrightnessMain *client, float *output, int x, int y, int is_brightness);
	~BrightnessSlider();
	int handle_event();
	char* get_caption();

	BrightnessMain *client;
	float *output;
	int is_brightness;
	char string[BCTEXTLEN];
};

class BrightnessLuma : public BC_CheckBox
{
public:
	BrightnessLuma(BrightnessMain *client, int x, int y);
	~BrightnessLuma();
	int handle_event();

	BrightnessMain *client;
};

#endif

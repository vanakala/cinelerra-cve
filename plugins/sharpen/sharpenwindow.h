#ifndef SHARPENWINDOW_H
#define SHARPENWINDOW_H

#include "guicast.h"

class SharpenThread;
class SharpenWindow;
class SharpenInterlace;

#include "filexml.h"
#include "mutex.h"
#include "sharpen.h"





PLUGIN_THREAD_HEADER(SharpenMain, SharpenThread, SharpenWindow)

class SharpenSlider;
class SharpenHorizontal;
class SharpenLuminance;

class SharpenWindow : public BC_Window
{
public:
	SharpenWindow(SharpenMain *client, int x, int y);
	~SharpenWindow();
	
	int create_objects();
	int close_event();
	
	SharpenMain *client;
	SharpenSlider *sharpen_slider;
	SharpenInterlace *sharpen_interlace;
	SharpenHorizontal *sharpen_horizontal;
	SharpenLuminance *sharpen_luminance;
};

class SharpenSlider : public BC_ISlider
{
public:
	SharpenSlider(SharpenMain *client, float *output, int x, int y);
	~SharpenSlider();
	int handle_event();

	SharpenMain *client;
	float *output;
};

class SharpenInterlace : public BC_CheckBox
{
public:
	SharpenInterlace(SharpenMain *client, int x, int y);
	~SharpenInterlace();
	int handle_event();

	SharpenMain *client;
};

class SharpenHorizontal : public BC_CheckBox
{
public:
	SharpenHorizontal(SharpenMain *client, int x, int y);
	~SharpenHorizontal();
	int handle_event();

	SharpenMain *client;
};

class SharpenLuminance : public BC_CheckBox
{
public:
	SharpenLuminance(SharpenMain *client, int x, int y);
	~SharpenLuminance();
	int handle_event();

	SharpenMain *client;
};


#endif

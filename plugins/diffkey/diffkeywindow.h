#ifndef DIFF_KEY_WINDOW_H
#define DIFF_KEY_WINDOW_H

//#define DEBUG

class DiffKeyThread;
class DiffKeyWindow;

#include "filexml.inc"
#include "diffkey.h"
#include "mutex.h"
#include "pluginvclient.h"

PLUGIN_THREAD_HEADER(DiffKeyMain, DiffKeyThread, DiffKeyWindow)

class DiffKeyToggle;
class DiffKeyResetButton;
class DiffKeyAddButton;
class DiffKeySlider;

class DiffKeyWindow : public BC_Window
{
public:
	DiffKeyWindow(DiffKeyMain *client, int x, int y);
	~DiffKeyWindow();
	
	int create_objects();
	int close_event();
	
	DiffKeyMain *client;
	DiffKeyResetButton *reset_key_frame;
	DiffKeyAddButton *add_key_frame;
	DiffKeySlider *hue_imp;
	DiffKeySlider *sat_imp;
	DiffKeySlider *val_imp;
	DiffKeySlider *r_imp;
	DiffKeySlider *g_imp;
	DiffKeySlider *b_imp;
	DiffKeySlider *vis_thresh;
	DiffKeySlider *trans_thresh;
	DiffKeySlider *desat_thresh;
	DiffKeyToggle *show_mask;
	DiffKeyToggle *hue_on;
	DiffKeyToggle *sat_on;
	DiffKeyToggle *val_on;
	DiffKeyToggle *r_on;
	DiffKeyToggle *g_on;
	DiffKeyToggle *b_on;
	DiffKeyToggle *vis_on;
	DiffKeyToggle *trans_on;
	DiffKeyToggle *desat_on;
};

class DiffKeyToggle : public BC_CheckBox
{
public:
	DiffKeyToggle(DiffKeyMain *client, int *output, char *string, int x, int y);
	~DiffKeyToggle();
	int handle_event();

	DiffKeyMain *client;
	int *output;
};


class DiffKeyResetButton : public BC_GenericButton
{
public:
	DiffKeyResetButton(DiffKeyMain *client, int x, int y);
	int handle_event();
	DiffKeyMain *client;
};

class DiffKeyAddButton : public BC_GenericButton
{
public:
	DiffKeyAddButton(DiffKeyMain *client, int x, int y);
	int handle_event();
	DiffKeyMain *client;
};

class DiffKeySlider : public BC_ISlider
{
public:
	DiffKeySlider(DiffKeyMain *client, float *output, int x, int y);
	int handle_event();

	DiffKeyMain *client;
	float *output;
};




#endif

#ifndef TRANSLATEWIN_H
#define TRANSLATEWIN_H

#include "guicast.h"

class TranslateThread;
class TranslateWin;

#include "filexml.h"
#include "mutex.h"
#include "pluginclient.h"
#include "translate.h"


PLUGIN_THREAD_HEADER(TranslateMain, TranslateThread, TranslateWin)

class TranslateCoord;

class TranslateWin : public BC_Window
{
public:
	TranslateWin(TranslateMain *client, int x, int y);
	~TranslateWin();

	int create_objects();
	int close_event();

	TranslateCoord *in_x, *in_y, *in_w, *in_h, *out_x, *out_y, *out_w, *out_h;
	TranslateMain *client;
};

class TranslateCoord : public BC_TumbleTextBox
{
public:
	TranslateCoord(TranslateWin *win, 
		TranslateMain *client, 
		int x, 
		int y, 
		float *value);
	~TranslateCoord();
	int handle_event();

	TranslateMain *client;
	TranslateWin *win;
	float *value;
};


#endif

#ifndef BLURWINDOW_H
#define BLURWINDOW_H


class BlurThread;
class BlurWindow;

#include "blur.inc"
#include "filexml.inc"
#include "guicast.h"
#include "mutex.h"
#include "thread.h"

PLUGIN_THREAD_HEADER(BlurMain, BlurThread, BlurWindow)

class BlurVertical;
class BlurHorizontal;
class BlurRadius;
class BlurA;
class BlurR;
class BlurG;
class BlurB;

class BlurWindow : public BC_Window
{
public:
	BlurWindow(BlurMain *client, int x, int y);
	~BlurWindow();
	
	int create_objects();
	int close_event();
	
	BlurMain *client;
	BlurVertical *vertical;
	BlurHorizontal *horizontal;
	BlurRadius *radius;
	BlurA *a;
	BlurR *r;
	BlurG *g;
	BlurB *b;
};

class BlurA : public BC_CheckBox
{
public:
	BlurA(BlurMain *client, int x, int y);
	int handle_event();
	BlurMain *client;
};
class BlurR : public BC_CheckBox
{
public:
	BlurR(BlurMain *client, int x, int y);
	int handle_event();
	BlurMain *client;
};
class BlurG : public BC_CheckBox
{
public:
	BlurG(BlurMain *client, int x, int y);
	int handle_event();
	BlurMain *client;
};
class BlurB : public BC_CheckBox
{
public:
	BlurB(BlurMain *client, int x, int y);
	int handle_event();
	BlurMain *client;
};


class BlurRadius : public BC_IPot
{
public:
	BlurRadius(BlurMain *client, int x, int y);
	~BlurRadius();
	int handle_event();

	BlurMain *client;
};

class BlurVertical : public BC_CheckBox
{
public:
	BlurVertical(BlurMain *client, BlurWindow *window, int x, int y);
	~BlurVertical();
	int handle_event();

	BlurMain *client;
	BlurWindow *window;
};

class BlurHorizontal : public BC_CheckBox
{
public:
	BlurHorizontal(BlurMain *client, BlurWindow *window, int x, int y);
	~BlurHorizontal();
	int handle_event();

	BlurMain *client;
	BlurWindow *window;
};


#endif

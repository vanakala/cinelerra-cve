#ifndef IVTCWINDOW_H
#define IVTCWINDOW_H

#include "guicast.h"

class IVTCThread;
class IVTCWindow;

#include "filexml.h"
#include "mutex.h"
#include "ivtc.h"


PLUGIN_THREAD_HEADER(IVTCMain, IVTCThread, IVTCWindow)


class IVTCOffset;
class IVTCFieldOrder;
class IVTCAuto;
class IVTCAutoThreshold;
class IVTCPattern;

class IVTCWindow : public BC_Window
{
public:
	IVTCWindow(IVTCMain *client, int x, int y);
	~IVTCWindow();
	
	int create_objects();
	int close_event();
	
	IVTCMain *client;
	IVTCOffset *frame_offset;
	IVTCFieldOrder *first_field;
	IVTCAuto *automatic;
	IVTCAutoThreshold *threshold;
	IVTCPattern *pattern[TOTAL_PATTERNS];
};

class IVTCOffset : public BC_TextBox
{
public:
	IVTCOffset(IVTCMain *client, int x, int y);
	~IVTCOffset();
	int handle_event();
	IVTCMain *client;
};

class IVTCFieldOrder : public BC_CheckBox
{
public:
	IVTCFieldOrder(IVTCMain *client, int x, int y);
	~IVTCFieldOrder();
	int handle_event();
	IVTCMain *client;
};

class IVTCAuto : public BC_CheckBox
{
public:
	IVTCAuto(IVTCMain *client, int x, int y);
	~IVTCAuto();
	int handle_event();
	IVTCMain *client;
};

class IVTCPattern : public BC_Radial
{
public:
	IVTCPattern(IVTCMain *client, 
		IVTCWindow *window, 
		int number, 
		char *text, 
		int x, 
		int y);
	~IVTCPattern();
	int handle_event();
	IVTCWindow *window;
	IVTCMain *client;
	int number;
};

class IVTCAutoThreshold : public BC_TextBox
{
public:
	IVTCAutoThreshold(IVTCMain *client, int x, int y);
	~IVTCAutoThreshold();
	int handle_event();
	IVTCMain *client;
};

#endif

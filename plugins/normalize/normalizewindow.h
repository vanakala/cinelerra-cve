#ifndef SRATEWINDOW_H
#define SRATEWINDOW_H

#include "guicast.h"

class NormalizeWindow : public BC_Window
{
public:
	NormalizeWindow(int x, int y);
	~NormalizeWindow();
	
	int create_objects(float *db_over, int *seperate_tracks);
	int close_event();
	
	float *db_over;
	int *separate_tracks;
};

class NormalizeWindowOverload : public BC_TextBox
{
public:
	NormalizeWindowOverload(int x, int y, float *db_over);
	~NormalizeWindowOverload();
	
	int handle_event();
	float *db_over;
};

class NormalizeWindowSeparate : public BC_CheckBox
{
public:
	NormalizeWindowSeparate(int x, int y, int *separate_tracks);
	~NormalizeWindowSeparate();
	
	int handle_event();
	int *separate_tracks;
};

#endif

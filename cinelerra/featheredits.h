#ifndef FEATHEREDITS_H
#define FEATHEREDITS_H

#include "guicast.h"
#include "mwindow.inc"

class FeatherEdits : public BC_MenuItem, Thread
{
public:
	FeatherEdits(MWindow *mwindow, int audio, int video);

	int handle_event();
	
	void run();

	MWindow *mwindow;
	int audio;
	int video;
};



class FeatherEditsTextBox;

class FeatherEditsWindow : public BC_Window
{
public:
	FeatherEditsWindow(MWindow *mwindow, long feather_samples);
	~FeatherEditsWindow();
	
	int create_objects(int audio, int video);

	long feather_samples;
	FeatherEditsTextBox  *text;
	int audio;
	int video;
};

class FeatherEditsTextBox : public BC_TextBox
{
public:
	FeatherEditsTextBox(FeatherEditsWindow *window, char *text, int x, int y);
	
	int handle_event();
	
	FeatherEditsWindow *window;
};


#endif

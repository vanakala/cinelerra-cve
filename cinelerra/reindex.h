#ifndef REINDEX_H
#define REINDEX_H

class ReIndex;

#include "indexfile.h"
#include "guicast.h"
#include "mainmenu.h"
#include "mwindow.h"
#include "thread.h"

class ReIndex : public BC_MenuItem, public Thread
{
public:
	ReIndex(MWindow *mwindow);
	~ReIndex();
	void run();
	handle_event();
	
	MWindow *mwindow;
};

class ReIndexOkButton;
class ReIndexCancelButton;
class ReIndexTextBox;

class ReIndexWindow : public BC_Window
{
public:
	ReIndexWindow(char *display = "");
	~ReIndexWindow();
	create_objects();
	
	MWindow *mwindow;
	long sample_rate;
	ReIndexOkButton *ok;
	ReIndexCancelButton *cancel;
};

class ReIndexOkButton : public BC_Button
{
public:
	ReIndexOkButton(ReIndexWindow *window);
	
	handle_event();
	keypress_event();
	
	ReIndexWindow *window;
};

class ReIndexCancelButton : public BC_Button
{
public:
	ReIndexCancelButton(ReIndexWindow *window);
	
	handle_event();
	keypress_event();
	
	ReIndexWindow *window;
};

#endif

#ifndef LOADMODE_H
#define LOADMODE_H

#include "guicast.h"
#include "loadmode.inc"
#include "mwindow.inc"

class LoadModeListBox;

class LoadModeItem : public BC_ListBoxItem
{
public:
	LoadModeItem(char *text, int value);
	int value;
};

class LoadMode
{
public:
	LoadMode(MWindow *mwindow,
		BC_WindowBase *window, 
		int x, 
		int y, 
		int *output,
		int use_nothing);
	~LoadMode();
	
	int create_objects();
	int reposition_window(int x, int y);

	char* mode_to_text();

	BC_Title *title;
	BC_TextBox *textbox;
	LoadModeListBox *listbox;
	MWindow *mwindow;
	BC_WindowBase *window;
	int x;
	int y;
	int *output;
	int use_nothing;
	ArrayList<LoadModeItem*> load_modes;
};

class LoadModeListBox : public BC_ListBox
{
public:
	LoadModeListBox(BC_WindowBase *window, LoadMode *loadmode, int x, int y);
	~LoadModeListBox();

	int handle_event();

	BC_WindowBase *window;
	LoadMode *loadmode;
};

#endif

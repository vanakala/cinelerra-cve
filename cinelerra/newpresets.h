#ifndef NEWPRESETS_H
#define NEWPRESETS_H


#include "guicast.h"
#include "new.inc"
#include "setformat.inc"

class NewPresetsText : public BC_TextBox
{
public:
	NewPresetsText(MWindow *mwindow, NewWindow *window, int x, int y);
	int handle_event();
	MWindow *mwindow;
	NewWindow *window;
};

class NewPresetsPulldown : public BC_ListBox
{
public:
	NewPresetsPulldown(MWindow *mwindow, NewWindow *window, int x, int y);
	int handle_event();
	MWindow *mwindow;
	NewWindow *window;
};

class NewPresetItem : public BC_ListBoxItem
{
public:
	NewPresetItem(MWindow *mwindow, NewWindow *window, char *text);
	~NewPresetItem();

	MWindow *mwindow;
	NewWindow *window;
	EDL *edl;
};





#endif

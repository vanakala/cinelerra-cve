#ifndef CONFIRMSAVE_H
#define CONFIRMSAVE_H

#include "asset.inc"
#include "guicast.h"
#include "mwindow.inc"

class ConfirmSaveOkButton;
class ConfirmSaveCancelButton;

class ConfirmSave
{
public:
	ConfirmSave();
	~ConfirmSave();

// Return values:
// 1 cancel
// 0 replace or doesn't exist yet
	static int test_file(MWindow *mwindow, char *path);
	static int test_files(MWindow *mwindow, ArrayList<char*> *paths);

};

class ConfirmSaveWindow : public BC_Window
{
public:
	ConfirmSaveWindow(MWindow *mwindow, ArrayList<BC_ListBoxItem*> *list);
	~ConfirmSaveWindow();

	int create_objects();
	int resize_event(int w, int h);

	ArrayList<BC_ListBoxItem*> *list;
	BC_Title *title;
	BC_ListBox *listbox;
};

#endif

#ifndef CONFIRMSAVE_H
#define CONFIRMSAVE_H

#include "guicast.h"
#include "mwindow.inc"

class ConfirmSaveOkButton;
class ConfirmSaveCancelButton;

class ConfirmSave
{
public:
	ConfirmSave(MWindow *mwindow);
	~ConfirmSave();

	int test_file(char *filename);    // return 1 if user cancels save
	MWindow *mwindow;
};

class ConfirmSaveWindow : public BC_Window
{
public:
	ConfirmSaveWindow(MWindow *mwindow, char *filename);
	~ConfirmSaveWindow();

	int create_objects();
	char *filename;
};

#endif

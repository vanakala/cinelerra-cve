#ifndef NEWFOLDER_H
#define NEWFOLDER_H

#include "awindowgui.inc"
#include "guicast.h"
#include "mutex.h"
#include "mwindow.inc"

class NewFolder : public BC_Window
{
public:
	NewFolder(MWindow *mwindow, AWindowGUI *awindow, int x, int y);
	~NewFolder();

	int create_objects();
	char* get_text();

private:
	BC_TextBox *textbox;
	MWindow *mwindow;
	AWindowGUI *awindow;
};


class NewFolderThread : public Thread
{
public:
	NewFolderThread(MWindow *mwindow, AWindowGUI *awindow);
	~NewFolderThread();

	void run();
	int interrupt();
	int start_new_folder();

private:
	Mutex change_lock, completion_lock;
	int active;
	MWindow *mwindow;
	AWindowGUI *awindow;
	NewFolder *window;
};

#endif

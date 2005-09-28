#ifndef BC_NEWFOLDER_H
#define BC_NEWFOLDER_H


#include "bcfilebox.inc"
#include "bcwindow.h"
#include "thread.h"


class BC_NewFolder : public BC_Window
{
public:
	BC_NewFolder(int x, int y, BC_FileBox *filebox);
	~BC_NewFolder();

	int create_objects();
	char* get_text();

private:
	BC_TextBox *textbox;
};

class BC_NewFolderThread : public Thread
{
public:
	BC_NewFolderThread(BC_FileBox *filebox);
	~BC_NewFolderThread();

	void run();
	int interrupt();
	int start_new_folder();

private:
	Mutex *change_lock;
	Condition *completion_lock;
	BC_FileBox *filebox;
	BC_NewFolder *window;
};





#endif

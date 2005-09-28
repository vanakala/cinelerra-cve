#ifndef BCDELETE_H
#define BCDELETE_H



#include "bcdialog.h"
#include "bcfilebox.inc"

class BC_DeleteFile : public BC_Window
{
public:
	BC_DeleteFile(BC_FileBox *filebox, int x, int y);
	~BC_DeleteFile();
	void create_objects();
	BC_FileBox *filebox;
	ArrayList<BC_ListBoxItem*> *data;
};

class BC_DeleteList : public BC_ListBox
{
public:
	BC_DeleteList(BC_FileBox *filebox, 
		int x, 
		int y, 
		int w, 
		int h,
		ArrayList<BC_ListBoxItem*> *data);
	BC_FileBox *filebox;
};

class BC_DeleteThread : public BC_DialogThread
{
public:
	BC_DeleteThread(BC_FileBox *filebox);
	void handle_done_event(int result);
	BC_Window* new_gui();

	BC_FileBox *filebox;
};








#endif

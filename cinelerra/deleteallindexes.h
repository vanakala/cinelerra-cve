#ifndef DELETEALLINDEXES_H
#define DELETEALLINDEXES_H

#include "guicast.h"
#include "mwindow.inc"
#include "preferencesthread.inc"
#include "thread.h"

class DeleteAllIndexes : public BC_GenericButton, public Thread
{
public:
	DeleteAllIndexes(MWindow *mwindow, PreferencesWindow *pwindow, int x, int y);
	~DeleteAllIndexes();
	
	void run();
	int handle_event();
	PreferencesWindow *pwindow;
	MWindow *mwindow;
};

class ConfirmDeleteAllIndexes : public BC_Window
{
public:
	ConfirmDeleteAllIndexes(MWindow *mwindow, char *string);
	~ConfirmDeleteAllIndexes();
	
	int create_objects();
	char *string;
};





#endif

#ifndef SAVEFILE_H
#define SAVEFILE_H

#include "guicast.h"
#include "mainmenu.inc"
#include "mwindow.inc"
#include "savefile.inc"

class SaveBackup : public BC_MenuItem
{
public:
	SaveBackup(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};

class Save : public BC_MenuItem
{
public:
	Save(MWindow *mwindow);
	int handle_event();
	int create_objects(SaveAs *saveas);
	int save_before_quit();
	
	int quit_now;
	MWindow *mwindow;
	SaveAs *saveas;
};

class SaveAs : public BC_MenuItem, public Thread
{
public:
	SaveAs(MWindow *mwindow);
	int set_mainmenu(MainMenu *mmenu);
	int handle_event();
	void run();
	
	int quit_now;
	MWindow *mwindow;
	MainMenu *mmenu;
};

class SaveFileWindow : public BC_FileBox
{
public:
	SaveFileWindow(MWindow *mwindow, char *init_directory);
	~SaveFileWindow();
	MWindow *mwindow;
};

#endif

#ifndef LOADFILE_H
#define LOADFILE_H

#include "guicast.h"
#include "loadmode.inc"
#include "mainmenu.inc"
#include "mwindow.inc"
#include "thread.h"

class LoadFileThread;
class LoadFileWindow;

class Load : public BC_MenuItem
{
public:
	Load(MWindow *mwindow, MainMenu *mainmenu);
	~Load();

	int create_objects();
	int handle_event();

	MWindow *mwindow;
	MainMenu *mainmenu;
	LoadFileThread *thread;
};

class LoadFileThread : public Thread
{
public:
	LoadFileThread(MWindow *mwindow, Load *menuitem);
	~LoadFileThread();

	void run();

	MWindow *mwindow;
	Load *load;
	int load_mode;
};

class NewTimeline;
class NewConcatenate;
class AppendNewTracks;
class EndofTracks;
class ResourcesOnly;

class LoadFileWindow : public BC_FileBox
{
public:
	LoadFileWindow(MWindow *mwindow, 
		LoadFileThread *thread,
		char *init_directory);
	~LoadFileWindow();

	int create_objects();
	int resize_event(int w, int h);

	LoadFileThread *thread;
	LoadMode *loadmode;
	MWindow *mwindow;
	NewTimeline *newtimeline;
	NewConcatenate *newconcatenate;
	AppendNewTracks *newtracks;
	EndofTracks *concatenate;
	ResourcesOnly *resourcesonly;
};

class NewTimeline : public BC_Radial
{
public:
	NewTimeline(int x, int y, LoadFileWindow *window);
	int handle_event();
	LoadFileWindow *window;
};

class NewConcatenate : public BC_Radial
{
public:
	NewConcatenate(int x, int y, LoadFileWindow *window);
	int handle_event();
	LoadFileWindow *window;
};

class AppendNewTracks : public BC_Radial
{
public:
	AppendNewTracks(int x, int y, LoadFileWindow *window);
	int handle_event();
	LoadFileWindow *window;
};

class EndofTracks : public BC_Radial
{
public:
	EndofTracks(int x, int y, LoadFileWindow *window);
	int handle_event();
	LoadFileWindow *window;
};

class ResourcesOnly : public BC_Radial
{
public:
	ResourcesOnly(int x, int y, LoadFileWindow *window);
	int handle_event();
	LoadFileWindow *window;
};



class LocateFileWindow : public BC_FileBox
{
public:
	LocateFileWindow(MWindow *mwindow, char *init_directory, char *old_filename);
	~LocateFileWindow();
	MWindow *mwindow;
};

class LoadPrevious : public BC_MenuItem, public Thread
{
public:
	LoadPrevious(MWindow *mwindow, Load *loadfile);
	int handle_event();
	void run();
	
	int set_path(char *path);
	
	MWindow *mwindow;
	Load *loadfile;
	char path[1024];
};

class LoadBackup : public BC_MenuItem
{
public:
	LoadBackup(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};

#endif

#include "assets.h"
#include "defaults.h"
#include "edl.h"
#include "errorbox.h"
#include "file.h"
#include "filesystem.h"
#include "indexfile.h"
#include "loadfile.h"
#include "loadmode.h"
#include "localsession.h"
#include "mainmenu.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"




#include <string.h>

Load::Load(MWindow *mwindow, MainMenu *mainmenu)
 : BC_MenuItem("Load files...", "o", 'o')
{ 
	this->mwindow = mwindow;
	this->mainmenu = mainmenu;
}

Load::~Load()
{
	delete thread;
}

int Load::create_objects()
{
	thread = new LoadFileThread(mwindow, this);
	return 0;
}

int Load::handle_event() 
{
//printf("Load::handle_event 1\n");
	if(!thread->running())
	{
//printf("Load::handle_event 2\n");
		thread->start();
	}
	return 1;
}






LoadFileThread::LoadFileThread(MWindow *mwindow, Load *load)
 : Thread()
{
	this->mwindow = mwindow;
	this->load = load;
}

LoadFileThread::~LoadFileThread()
{
}

void LoadFileThread::run()
{
	int result;
	ArrayList<BC_ListBoxItem*> *dirlist;
	FileSystem fs;
	ArrayList<char*> path_list;
	char default_path[1024];

	sprintf(default_path, "~");
	mwindow->defaults->get("DEFAULT_LOADPATH", default_path);
	load_mode = mwindow->defaults->get("LOAD_MODE", LOAD_REPLACE);

	{
		LoadFileWindow window(mwindow, this, default_path);
		window.create_objects();
		result = window.run_window();

// Collect all selected files
		if(!result)
		{
			char *in_path, *out_path;
			int i = 0;

			while(in_path = window.get_path(i))
			{
				int j;
				for(j = 0; j < path_list.total; j++)
				{
					if(!strcmp(in_path, path_list.values[j])) break;
				}
				
				if(j == path_list.total)
				{
					path_list.append(out_path = new char[strlen(in_path) + 1]);
					strcpy(out_path, in_path);
				}
				i++;
			}
		}

		mwindow->defaults->update("DEFAULT_LOADPATH", window.get_path());
		mwindow->defaults->update("LOAD_MODE", load_mode);
	}

// No file selected
	if(path_list.total == 0 || result == 1)
	{
		return;
	}

	mwindow->undo->update_undo_before("load", LOAD_ALL);
	mwindow->interrupt_indexes();
	mwindow->gui->lock_window();
	result = mwindow->load_filenames(&path_list, load_mode);
	mwindow->gui->mainmenu->add_load(path_list.values[0]);
	mwindow->gui->unlock_window();
	path_list.remove_all_objects();


	mwindow->save_backup();
	mwindow->restart_brender();
	mwindow->undo->update_undo_after();
	return;
}








LoadFileWindow::LoadFileWindow(MWindow *mwindow, 
	LoadFileThread *thread,
	char *init_directory)
 : BC_FileBox(mwindow->gui->get_abs_cursor_x(),
 		mwindow->gui->get_abs_cursor_y() - BC_WindowBase::get_resources()->filebox_h / 2,
		init_directory, 
		PROGRAM_NAME ": Load",
		"Select files to load:", 
		0,
		0,
		1,
		mwindow->theme->loadfile_pad)
{
	this->thread = thread;
	this->mwindow = mwindow; 
}

LoadFileWindow::~LoadFileWindow() 
{
	delete loadmode;
}

int LoadFileWindow::create_objects()
{
	BC_FileBox::create_objects();

	int x = get_w() / 2 - 200;
	int y = get_h() - 90;
	loadmode = new LoadMode(mwindow, this, x, y, &thread->load_mode, 0);
	loadmode->create_objects();

	return 0;
}

int LoadFileWindow::resize_event(int w, int h)
{
	int x = w / 2 - 200;
	int y = h - 90;
	draw_background(0, 0, w, h);

	loadmode->reposition_window(x, y);

	return BC_FileBox::resize_event(w, h);
}


NewTimeline::NewTimeline(int x, int y, LoadFileWindow *window)
 : BC_Radial(x, 
 	y, 
	window->thread->load_mode == LOAD_REPLACE,
	"Replace current project.")
{
	this->window = window;
}
int NewTimeline::handle_event()
{
	window->newtracks->set_value(0);
	window->newconcatenate->set_value(0);
	window->concatenate->set_value(0);
	window->resourcesonly->set_value(0);
	return 1;
}

NewConcatenate::NewConcatenate(int x, int y, LoadFileWindow *window)
 : BC_Radial(x, 
 	y, 
	window->thread->load_mode == LOAD_REPLACE_CONCATENATE,
	"Replace current project and concatenate tracks.")
{
	this->window = window;
}
int NewConcatenate::handle_event()
{
	window->newtimeline->set_value(0);
	window->newtracks->set_value(0);
	window->concatenate->set_value(0);
	window->resourcesonly->set_value(0);
	return 1;
}

AppendNewTracks::AppendNewTracks(int x, int y, LoadFileWindow *window)
 : BC_Radial(x, 
 	y, 
	window->thread->load_mode == LOAD_NEW_TRACKS,
	"Append in new tracks.")
{
	this->window = window;
}
int AppendNewTracks::handle_event()
{
	window->newtimeline->set_value(0);
	window->newconcatenate->set_value(0);
	window->concatenate->set_value(0);
	window->resourcesonly->set_value(0);
	return 1;
}

EndofTracks::EndofTracks(int x, int y, LoadFileWindow *window)
 : BC_Radial(x, 
 	y, 
	window->thread->load_mode == LOAD_CONCATENATE,
	"Concatenate to existing tracks.")
{
	this->window = window;
}
int EndofTracks::handle_event()
{
	window->newtimeline->set_value(0);
	window->newconcatenate->set_value(0);
	window->newtracks->set_value(0);
	window->resourcesonly->set_value(0);
	return 1;
}

ResourcesOnly::ResourcesOnly(int x, int y, LoadFileWindow *window)
 : BC_Radial(x, 
 	y, 
	window->thread->load_mode == LOAD_RESOURCESONLY,
	"Create new resources only.")
{
	this->window = window;
}
int ResourcesOnly::handle_event()
{
	set_value(1);
	window->newtimeline->set_value(0);
	window->newconcatenate->set_value(0);
	window->newtracks->set_value(0);
	window->concatenate->set_value(0);
	return 1;
}







LocateFileWindow::LocateFileWindow(MWindow *mwindow, 
	char *init_directory, 
	char *old_filename)
 : BC_FileBox(mwindow->gui->get_abs_cursor_x(),
 		mwindow->gui->get_abs_cursor_y(), 
		init_directory, 
		PROGRAM_NAME ": Locate file", 
		old_filename)
{ 
	this->mwindow = mwindow; 
}

LocateFileWindow::~LocateFileWindow() {}







LoadPrevious::LoadPrevious(MWindow *mwindow, Load *loadfile)
 : BC_MenuItem(""), Thread()
{ 
	this->mwindow = mwindow;
	this->loadfile = loadfile; 
}

int LoadPrevious::handle_event()
{
	ArrayList<char*> path_list;
	char *out_path;
	int load_mode = mwindow->defaults->get("LOAD_MODE", LOAD_REPLACE);

	mwindow->undo->update_undo_before("load previous", LOAD_ALL);

	path_list.append(out_path = new char[strlen(path) + 1]);
	strcpy(out_path, path);
//printf("LoadPrevious::LoadPrevious 1\n");
	mwindow->load_filenames(&path_list, LOAD_REPLACE);
//printf("LoadPrevious::LoadPrevious 2\n");
	mwindow->gui->mainmenu->add_load(path_list.values[0]);
//printf("LoadPrevious::LoadPrevious 3\n");
	path_list.remove_all_objects();
//printf("LoadPrevious::LoadPrevious 4\n");


	mwindow->defaults->update("LOAD_MODE", load_mode);
	mwindow->undo->update_undo_after();
	mwindow->save_backup();
	return 1;
}







void LoadPrevious::run()
{
//	loadfile->mwindow->load(path, loadfile->append);
}

int LoadPrevious::set_path(char *path)
{
	strcpy(this->path, path);
}








LoadBackup::LoadBackup(MWindow *mwindow)
 : BC_MenuItem("Load backup")
{
	this->mwindow = mwindow;
}

int LoadBackup::handle_event()
{
	ArrayList<char*> path_list;
	char *out_path;
	char string[BCTEXTLEN];
	strcpy(string, BACKUP_PATH);
	FileSystem fs;
	fs.complete_path(string);
	
	path_list.append(out_path = new char[strlen(string) + 1]);
	strcpy(out_path, string);
	
	mwindow->undo->update_undo_before("load backup", LOAD_ALL);
	mwindow->load_filenames(&path_list, LOAD_REPLACE);
	mwindow->edl->local_session->clip_title[0] = 0;
	mwindow->set_filename("");
	path_list.remove_all_objects();
	mwindow->undo->update_undo_after();
	mwindow->save_backup();

	return 1;
}
	




